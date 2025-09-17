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

	ableton::Link link;
	double realBpm;
	double expectedPhase;
	LinkCompensation compensationMode;
	mp_sint32 straddleDirection = 0;
	bool beatRequested = false;
	mp_sint32 quantum = 4;
	mp_sint32 ticksToSkip = 0;

public:
	LinkContext(mp_sint32 bpm);

	void setCompensation(LinkCompensation mode);
	LinkCompensation getCompensation();

	void setBpm(double bpm);

	void mixHandler(PlayerBase *player);

	void skipTick(PlayerBase *player);
	void tick();
};

#endif
#endif
