#pragma once

// packets graphic packs can send to host modules, other Cemu builds just skip them

void LatteHostEvent_itHostEvent(const uint32be* data, uint32 nWords);
void LatteHostEvent_itSetView(const uint32be* data, uint32 nWords);
bool LatteHostEvent_itViewAluConst(const uint32be* data, uint32 nWords, bool& isVertexStage);
