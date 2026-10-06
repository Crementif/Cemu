#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"

// queries get split up per render pass, multiview passes need an index per view
class LatteQueryObjectVk : public LatteQueryObject
{
	friend class VulkanRenderer;

	LatteQueryObjectVk(VulkanRenderer* rendererVk) : m_rendererVk(rendererVk)
	{

	};

	bool getResult(uint64& numSamplesPassed) override;
	void begin() override;
	void end() override;
	void beginFragment(uint32 viewCount);
	void endFragment();
	void handleFinishedFragments();

private:
	struct queryFragment
	{
		uint32 queryIndex;
		uint32 viewCount;
		uint64 m_finishCommandBuffer;
		bool isFinished;
	};

	VulkanRenderer* m_rendererVk;
	std::vector<queryFragment> list_queryFragments;
	bool m_vkQueryEnded{};
	bool m_hasActiveQuery{};
	bool m_hasActiveFragment{};
	uint64 m_finishCommandBuffer;
	uint64 m_acccumulatedSum;
};

bool LatteQueryObjectVk::getResult(uint64& numSamplesPassed)
{
	if (!m_vkQueryEnded)
		return false;
	if (!m_rendererVk->HasCommandBufferFinished(m_finishCommandBuffer))
		return false;
	handleFinishedFragments();
	cemu_assert_debug(list_queryFragments.empty());
	numSamplesPassed = m_acccumulatedSum;
	return true;
}

void LatteQueryObjectVk::beginFragment(uint32 viewCount)
{
	handleFinishedFragments();

	queryFragment qf{};
	qf.queryIndex = m_rendererVk->occlusionQuery_acquireIndices(viewCount);
	qf.viewCount = viewCount;
	qf.isFinished = false;
	qf.m_finishCommandBuffer = 0;
	list_queryFragments.emplace_back(qf);

	vkCmdBeginQuery(m_rendererVk->m_state.currentCommandBuffer, m_rendererVk->m_occlusionQueries.queryPool, qf.queryIndex, VK_QUERY_CONTROL_PRECISE_BIT);
	m_hasActiveFragment = true;
}

void LatteQueryObjectVk::begin()
{
	m_vkQueryEnded = false;
	m_hasActiveQuery = true;
	if (!m_rendererVk->m_state.activeRenderpassFBO)
		return;
	// indices can only be reset outside of render passes
	if (m_rendererVk->m_occlusionQueries.list_resetQueryIndices.size() < m_rendererVk->m_occlusionQueries.passViewCount)
		m_rendererVk->draw_endRenderPass();
	else
		beginFragment(m_rendererVk->m_occlusionQueries.passViewCount);
}

void LatteQueryObjectVk::endFragment()
{
	cemu_assert_debug(m_hasActiveFragment);
	vkCmdEndQuery(m_rendererVk->m_state.currentCommandBuffer, m_rendererVk->m_occlusionQueries.queryPool, list_queryFragments.back().queryIndex);
	list_queryFragments.back().m_finishCommandBuffer = m_rendererVk->GetCurrentCommandBufferId();
	list_queryFragments.back().isFinished = true;
	m_hasActiveFragment = false;
}

void LatteQueryObjectVk::handleFinishedFragments()
{
	// remove finished fragments and add to m_acccumulatedSum
	while (!list_queryFragments.empty())
	{
		auto& it = list_queryFragments.front();
		if (!it.isFinished)
			break;
		if (!m_rendererVk->HasCommandBufferFinished(it.m_finishCommandBuffer))
			break;
		// some drivers put the total of all views in the first index
		uint64 samples = 0;
		vkGetQueryPoolResults(m_rendererVk->m_logicalDevice, m_rendererVk->m_occlusionQueries.queryPool, it.queryIndex, 1, sizeof(samples), &samples, sizeof(samples), VK_QUERY_RESULT_64_BIT);
		m_acccumulatedSum += samples;
		m_rendererVk->occlusionQuery_releaseIndices(it.queryIndex, it.viewCount);
		list_queryFragments.erase(list_queryFragments.begin());
	}
}

void LatteQueryObjectVk::end()
{
	if(m_hasActiveFragment)
		endFragment();
	m_vkQueryEnded = true;
	m_hasActiveQuery = false;
	m_finishCommandBuffer = m_rendererVk->GetCurrentCommandBufferId();
	m_rendererVk->m_occlusionQueries.m_lastCommandBuffer = m_finishCommandBuffer;
	m_rendererVk->RequestSubmitSoon(); // make sure the current command buffer gets submitted soon
	m_rendererVk->RequestSubmitOnIdle();
}

LatteQueryObject* VulkanRenderer::occlusionQuery_create()
{
	// create query pool if it doesn't already exist
	if(m_occlusionQueries.queryPool == VK_NULL_HANDLE)
	{
		VkQueryPoolCreateInfo queryPoolCreateInfo{};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.flags = 0;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
		queryPoolCreateInfo.queryCount = OCCLUSION_QUERY_POOL_SIZE;
		queryPoolCreateInfo.pipelineStatistics = 0;
		auto r = vkCreateQueryPool(m_logicalDevice, &queryPoolCreateInfo, nullptr, &m_occlusionQueries.queryPool);
		if (r != VK_SUCCESS)
		{
			cemuLog_log(LogType::Force, "Vulkan-Error: Failed to create query pool with error {}", (sint32)r);
			return nullptr;
		}
	}
	LatteQueryObjectVk* queryObjVk = nullptr;
	if (m_occlusionQueries.list_cachedQueries.empty())
	{
		queryObjVk = new LatteQueryObjectVk(this);
	}
	else
	{
		queryObjVk = m_occlusionQueries.list_cachedQueries.front();
		m_occlusionQueries.list_cachedQueries.erase(m_occlusionQueries.list_cachedQueries.begin()+0);
	}
	queryObjVk->queryEnded = false;
	queryObjVk->queryEventStart = 0;
	queryObjVk->queryEventEnd = 0;
	queryObjVk->m_vkQueryEnded = false;
	queryObjVk->m_acccumulatedSum = 0;
	cemu_assert_debug(queryObjVk->list_queryFragments.empty()); // query fragment list should always be cleared in _destroy()
	m_occlusionQueries.list_currentlyActiveQueries.emplace_back(queryObjVk);
	return queryObjVk;
}

void VulkanRenderer::occlusionQuery_destroy(LatteQueryObject* queryObj)
{
	LatteQueryObjectVk* queryObjVk = static_cast<LatteQueryObjectVk*>(queryObj);
	m_occlusionQueries.list_currentlyActiveQueries.erase(std::remove(m_occlusionQueries.list_currentlyActiveQueries.begin(), m_occlusionQueries.list_currentlyActiveQueries.end(), queryObj), m_occlusionQueries.list_currentlyActiveQueries.end());
	m_occlusionQueries.list_cachedQueries.emplace_back(queryObjVk);
	for (auto& it : queryObjVk->list_queryFragments)
		occlusionQuery_releaseIndices(it.queryIndex, it.viewCount);
	queryObjVk->list_queryFragments.clear();
}

void VulkanRenderer::occlusionQuery_flush()
{
	WaitCommandBufferFinished(m_occlusionQueries.m_lastCommandBuffer);
}

void VulkanRenderer::occlusionQuery_updateState()
{
	// check for finished command buffers here since query states are tied to buffers
	ProcessFinishedCommandBuffers();
}

void VulkanRenderer::occlusionQuery_resetReleasedIndices()
{
	auto& released = m_occlusionQueries.list_releasedQueryIndices;
	if (released.empty())
		return;
	std::sort(released.begin(), released.end());
	size_t rangeBegin = 0;
	for (size_t i = 1; i <= released.size(); i++)
	{
		if (i < released.size() && released[i] == released[i - 1] + 1)
			continue;
		vkCmdResetQueryPool(m_state.currentCommandBuffer, m_occlusionQueries.queryPool, released[rangeBegin], (uint32)(i - rangeBegin));
		rangeBegin = i;
	}
	m_occlusionQueries.list_resetQueryIndices.insert(released.begin(), released.end());
	released.clear();
}

void VulkanRenderer::occlusionQuery_prepareRenderPass(uint32 viewCount)
{
	m_occlusionQueries.passViewCount = viewCount;
	if (m_occlusionQueries.queryPool == VK_NULL_HANDLE)
		return;
	if (m_occlusionQueries.list_resetQueryIndices.size() < (m_occlusionQueries.list_currentlyActiveQueries.size() + 1) * viewCount)
		occlusionQuery_resetReleasedIndices();
}

void VulkanRenderer::occlusionQuery_notifyBeginRenderPass()
{
	for (auto& it : m_occlusionQueries.list_currentlyActiveQueries)
		if (it->m_hasActiveQuery)
			it->beginFragment(m_occlusionQueries.passViewCount);
}

void VulkanRenderer::occlusionQuery_notifyEndRenderPass()
{
	for (auto& it : m_occlusionQueries.list_currentlyActiveQueries)
		if (it->m_hasActiveFragment)
			it->endFragment();
}

uint32 VulkanRenderer::occlusionQuery_acquireIndices(uint32 count)
{
	auto& reset = m_occlusionQueries.list_resetQueryIndices;
	for (auto it = reset.begin(); it != reset.end(); ++it)
	{
		uint32 first = *it;
		auto last = it;
		uint32 n = 1;
		while (n < count && std::next(last) != reset.end() && *std::next(last) == first + n)
		{
			++last;
			n++;
		}
		if (n == count)
		{
			reset.erase(it, std::next(last));
			return first;
		}
	}
	cemuLog_log(LogType::Force, "Vulkan-Error: Exhausted query pool");
	assert_dbg();
	return 0;
}

void VulkanRenderer::occlusionQuery_releaseIndices(uint32 firstIndex, uint32 count)
{
	for (uint32 i = 0; i < count; i++)
		m_occlusionQueries.list_releasedQueryIndices.emplace_back((uint16)(firstIndex + i));
}
