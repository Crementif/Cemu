#pragma once

void LatteTiming_setCustomVsyncFrequency(sint32 frequency);
void LatteTiming_disableCustomVsyncFrequency();
bool LatteTiming_getCustomVsyncFrequency(sint32& customFrequency);
void LatteTiming_setHostModuleVsyncFrequency(sint32 frequency);
bool LatteTiming_hasHostModuleVsyncFrequency();

void LatteTiming_EnableHostDrivenVSync();