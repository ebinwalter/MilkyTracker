#include "LinkContext.h"
#include "PlayerBase.h"
#include <math.h>

LinkContext::LinkContext(mp_sint32 bpm) :
	link((double) bpm) 
{
	link.enable(true);
	link.enableStartStopSync(true);
	realBpm = (double) bpm;
	compensationMode = SkipOrAddTicks;
	beatRequested = false;
}

void LinkContext::setCompensation(LinkCompensation mode) {
	compensationMode = mode;
}

LinkContext::LinkCompensation LinkContext::getCompensation() {
	return compensationMode;
}

void LinkContext::skipTick(PlayerBase *player) {
	if (++player->ticker != player->tickSpeed)
		expectedPhase += 1.0/24.0;
}

void LinkContext::tick() {
	expectedPhase += 1.0/24.0;
	if (expectedPhase >= quantum)
		expectedPhase -= quantum;
}

void LinkContext::mixHandler(PlayerBase *player) {
	ableton::Link::SessionState state = link.captureAudioSessionState();
	auto microsNow = link.clock().micros();
	double linkTempo = state.tempo();

	// Negotiate tempo
	if (realBpm != linkTempo) {
		realBpm = linkTempo;
		player->setTempo((mp_sint32) floor(linkTempo));
		printf("Set tempo to %dbpm", player->getTempo());
	}

	if(state.isPlaying()) {
		// We need to prepare to play if halted or idle
		if (player->halted || player->idle) {
			if (!beatRequested)
			{
				beatRequested = true;
				state.requestBeatAtStartPlayingTime(0, quantum);
				link.commitAudioSessionState(state);
			}
			if (state.timeAtBeat(0, quantum) <= microsNow) {
				expectedPhase = 0.0;
				player->startPlaying(player->module, true);
			}
		} else {
			double linkPhase = state.phaseAtTime(microsNow, quantum);
			double drift = expectedPhase - linkPhase;

			if (drift < -(float) quantum / 2.0)
				drift += (float) quantum;

			printf("Current drift: %f\n", drift);
			if (drift < -0.03)
				skipTick(player);	
		}
	}
}
