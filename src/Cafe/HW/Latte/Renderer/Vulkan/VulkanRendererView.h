#pragma once

class CachedFBOVk;
class LatteTexture;
class LatteQueryObjectVk;
struct VkDescriptorSetInfo;
struct LatteDecompilerShader;

enum class VulkanViewCommandType : uint8
{
	BindPipeline,
	BindIndexBuffer,
	BindVertexBuffer,
	SetViewport,
	SetScissor,
	SetDepthBias,
	SetBlendConstants,
	SetFeedbackLoop,
	Barrier,
	BindDescriptorSet,
	Draw,
	DrawIndexed,
};

struct VulkanViewCommand
{
	VulkanViewCommandType type;
	union
	{
		VkPipeline pipeline;
		struct
		{
			VkBuffer buffer;
			VkDeviceSize offset;
			VkIndexType indexType;
		}indexBuffer;
		struct
		{
			uint32 index;
			VkBuffer buffer;
			VkDeviceSize offset;
		}vertexBuffer;
		VkViewport viewport;
		VkRect2D scissor;
		float depthBias[3]; // constant, clamp, slope
		float blendConstants[4];
		VkImageAspectFlags feedbackLoopAspect;
		struct
		{
			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;
			VkAccessFlags srcAccess;
			VkAccessFlags dstAccess;
			VkDependencyFlags dependencyFlags;
		}barrier;
		struct
		{
			VkPipelineLayout layout;
			uint32 setIndex;
			VkDescriptorSet set;
			uint32 dynamicOffsetStart;
			uint32 dynamicOffsetCount;
		}descriptorSet;
		struct
		{
			uint32 count;
			uint32 instanceCount;
			uint32 baseVertex;
			uint32 baseInstance;
		}draw;
	};
};

struct VulkanViewStageBinding
{
	VkDescriptorSet set{};
	uint32 dynamicOffsets[17]{};
	sint32 dynamicOffsetCount{-1};
};

struct VulkanViewUniformData
{
	LatteDecompilerShader* shader{};
	uint32 leftOffset{};
	uint32 rightDataEpoch{};
	uint32 rightOffset{};
	uint64 commandBufferId{};
};

struct VulkanViewState
{
bool isStereo{};
	bool isRecording{};
	bool drawNeedsRight{};
	bool hasNoRightPass{};
	bool restoreBindings{};
	bool hasRightPass{};
	VkRenderPass renderPass{};
	VkFramebuffer framebuffer{};
	VkExtent2D extent{};
	CachedFBOVk* rightFBO{};
	VkRenderingInfoKHR renderingInfo{};
	VkRenderingAttachmentInfoKHR colorAttachments[8]{};
	VkRenderingAttachmentInfoKHR depthAttachment{};
	VkRenderingAttachmentInfoKHR stencilAttachment{};
	std::vector<VulkanViewCommand> commands;
	std::vector<uint32> dynamicOffsets;
	std::vector<std::pair<LatteTexture*, uint32>> pendingSeeds; // texture and slice/mip index
	struct
	{
		VkBuffer buffer{};
		VkDeviceSize offset{};
	}vertexBinding[Latte::GPU_LIMITS::NUM_VERTEX_BUFFERS];
	VkBuffer indexBuffer{};
	VkDeviceSize indexBufferOffset{};
	VkIndexType indexType{};
	float depthBias[3]{};
	float blendConstants[4]{};
	struct
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
		VkBuffer indexBuffer;
		VkDeviceSize indexBufferOffset;
		VkIndexType indexType;
		VkBuffer vertexBuffer[Latte::GPU_LIMITS::NUM_VERTEX_BUFFERS];
		VkDeviceSize vertexOffset[Latte::GPU_LIMITS::NUM_VERTEX_BUFFERS];
		VkViewport viewport;
		VkRect2D scissor;
		float depthBias[3];
		float blendConstants[4];
		VkImageAspectFlags feedbackLoopAspect;
		VulkanViewStageBinding stage[(size_t)LatteConst::ShaderType::TotalCount];
	}bound;
	std::unordered_map<CachedFBOVk*, CachedFBOVk*> rightFBOs;
	struct RightDescriptorSet
	{
		uint32 siblingEpoch;
		VkDescriptorSetInfo* set;
	};
	std::unordered_map<VkDescriptorSetInfo*, RightDescriptorSet> rightDescriptorSets;
	std::unordered_set<VkDescriptorSetInfo*> knownRightDescriptorSets;
	// multiview
	CachedFBOVk* guestFBO{};
	bool isMultiviewFBO{};
	bool buildPairDescriptorSet{};
	std::unordered_map<CachedFBOVk*, CachedFBOVk*> multiviewFBOs;
	std::unordered_map<VkDescriptorSetInfo*, RightDescriptorSet> pairDescriptorSets;
	std::unordered_set<VkDescriptorSetInfo*> knownPairDescriptorSets;
	VulkanViewUniformData uniformData[(size_t)LatteConst::ShaderType::TotalCount];
};
