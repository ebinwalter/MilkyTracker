#ifdef HAS_LINK
#ifndef LINKCONTEXT_H
#define LINKCONTEXT_H

class PlayerBase;
class PlayerIT;
class PlayerSTD;
class PlayerFAR;

#include "MilkyPlayTypes.h"
#include <ableton/Link.hpp>

class LinkContext {
private:
	enum LinkCompensation {
		SkipOrAddTicks,
		StraddleBpm
	};

	PlayerBase *player;
	ableton::Link link;
	double realBpm;
	double expectedPhase = 0.0;
	LinkCompensation compensationMode;
	mp_sint32 straddleDirection = 0;
	bool beatRequested = false;
	mp_sint32 quantum = 4;
	mp_sint32 ticksToSkip = 0;
	bool iPaused = false;
	bool amPlaying = false;

public:
	LinkContext(PlayerBase *player, mp_sint32 bpm);

	// The following are functions which change the state of the player itself.
	// These allow us to change the state of the player without automatically
	// triggering an update to the Link session.
	
	// Set the BPM of the player 
	void setPlayerBpm(double bpm);
	// Skip a tick because we're behind the Link clock
	void skipTick();
	// Pause the player in response to a Link event
	void pausePlayer();
	// Start the player in response to a Link event
	void startPlayer();

	// Handlers of other events
	// Called when the ticker increments
	void onTick();
	// Called when the player pauses of its own accord
	void onPause();
	// Called when the player halts
	void onHalt();
	// Called every time a block of audio is mixed
	void onMix();
	// Called when we request that playback starts (i.e., the start
	// event wasn't caused by our Link code)
	void onStart();
	// Called when _our module_ causes the BPM to change
	void onTempoChange(mp_sint32 bpm);

	/* Disabling copy and move because Link can't be moved either. */
	LinkContext(const LinkContext&) = delete;
	LinkContext& operator=(const LinkContext&) = delete;
};

#endif
#endif
