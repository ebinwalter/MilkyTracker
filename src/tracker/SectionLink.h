#if !defined(SECTIONLINK__H)
#define SECTIONLINK__H

#include "BasicTypes.h"
#include "Event.h"
#include "SectionAbstract.h"

class PPControl;
class LinkControl;

class SectionLink : public SectionAbstract
{
private:
	PPContainer* containerEntire;
	PPContainer* containerBottom;
	bool visible;

protected:
	virtual void showSection(bool bShow);

public:
	SectionLink(Tracker& theTracker);
	~SectionLink();

	pp_int32 handleEvent(PPObject* sender, PPEvent* event);

	void init();
	void init(pp_int32 x, pp_int32 y);

	void show(bool bShow);
	void update(bool repaint = true);

	void realign();
};

#endif
