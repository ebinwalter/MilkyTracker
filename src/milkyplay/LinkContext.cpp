#include "LinkContext.h"
#include "PlayerBase.h"
#include "XModule.h"
#include <ableton/link/Controller.hpp>
#include <math.h>

LinkContext::LinkContext(PlayerBase *player, mp_sint32 bpm) :
	link((double) bpm),
	player(player)
{
	link.enable(true);
	link.enableStartStopSync(true);
	compensationMode = SkipOrAddTicks;
	beatRequested = false;
	realBpm = bpm;
}

void LinkContext::pausePlayer()
{
	player->paused = true;
}

void LinkContext::startPlayer()
{
	((ChannelMixer *) player)->setNumChannels(player->initialNumChannels);

	player->idle = false;
	player->repeat = true;
	
	mp_sint32 res = player->allocateStructures();

	player->patternIndexToPlay = -1;

	player->restart();
	player->kick();
}

void LinkContext::setPlayerBpm(double bpm) 
{
	// This does what player->setTempo() does, but doesn't trigger
	// a re-broadcasting of the tempo on the Link session and blow
	// up the call stack.
	player->bpm = round(bpm);
	realBpm = bpm;
	player->updateAdder();
	player->reallocTimeRecord();
}

void LinkContext::skipTick() 
{
	if (++player->ticker != player->tickSpeed)
		expectedPhase += 1.0/24.0;
	if (expectedPhase >= quantum)
		expectedPhase -= quantum;
}

void LinkContext::onTick() 
{
	expectedPhase += 1.0/24.0;
	if (expectedPhase >= quantum)
		expectedPhase -= quantum;
}

void LinkContext::onPause() 
{
	printf("Committing pause to link state\n");
	auto state = link.captureAudioSessionState();
	auto now = link.clock().micros();
	state.setIsPlaying(false, now + std::chrono::milliseconds(10));
	link.commitAudioSessionState(state);
}

void LinkContext::onStart() {
	auto state = link.captureAudioSessionState();
	state.setIsPlayingAndRequestBeatAtTime(
		true,
		link.clock().micros(),
		0,
		quantum
	);
	amPlaying = false;
	link.commitAudioSessionState(state);
	beatRequested = true;
}

void LinkContext::onTempoChange(mp_sint32 bpm) 
{
	auto state = link.captureAudioSessionState();
	auto now = link.clock().micros();
	realBpm = (double) bpm;
	state.setTempo(realBpm, now);
	link.commitAudioSessionState(state);
}

void LinkContext::onMix() 
{
	ableton::Link::SessionState state = link.captureAudioSessionState();
	auto microsNow = link.clock().micros();
	double linkTempo = state.tempo();

	if(linkTempo != realBpm && amPlaying) {
		setPlayerBpm(linkTempo);
	}

	if(state.isPlaying()) {
		// We need to prepare to play if halted or idle
		if (!amPlaying) {
			if (!beatRequested)
			{
				beatRequested = true;
				state.requestBeatAtStartPlayingTime(0, quantum);
				link.commitAudioSessionState(state);
				// Force sync bpm whenever we request the downbeat
				// so downbeat is not off time 
			}
			if (state.timeAtBeat(0, quantum) <= microsNow) {
				expectedPhase = 0.0;
				amPlaying = true;
				beatRequested = false;
				startPlayer();
				setPlayerBpm(linkTempo);
			}
		} else {
			double linkPhase = state.phaseAtTime(microsNow, quantum);
			double drift = expectedPhase - linkPhase;

			if (drift < -(float) quantum / 2.0)
				drift += (float) quantum;

			if (drift < -0.03)
				skipTick();	
		}
	} else if (!state.isPlaying()) {
		if (amPlaying && state.timeForIsPlaying() <= microsNow) {
			pausePlayer();
		}
	}
}
