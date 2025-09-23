#include "SectionLink.h"
#include "StaticText.h"
#include "TrackerConfig.h"
#include "PatternEditorControl.h"
#include "Container.h"
#include "TransparentContainer.h"
#include "ControlIDs.h"
SectionLink::SectionLink(Tracker& theTracker) :
	SectionAbstract(theTracker, NULL),
	containerEntire(NULL),
	visible(false)
{
}

SectionLink::~SectionLink()
{

}

void SectionLink::show(bool bShow) 
{
	SectionAbstract::show(bShow);
	visible = bShow;
	containerEntire->show(bShow);
	if (!initialised)
		init();
	PatternEditorControl* control = tracker.getPatternEditorControl();
	realign();
	if (bShow)
	{
		if (control)
			tracker.hideInputControl();
		update(false);
	}
	showSection(bShow);
}

// blatantly lifted from SectionInstruments::realign
void SectionLink::realign() {
	pp_uint32 maxShould = tracker.MAXEDITORHEIGHT();
	pp_uint32 maxIs = containerEntire->getLocation().y + containerEntire->getSize().height;
	
	if (maxIs != maxShould)
	{
		pp_int32 offset = maxShould - maxIs;
		containerEntire->move(PPPoint(0, offset));
	}
	
	PatternEditorControl* control = tracker.getPatternEditorControl();
	PPScreen* screen = tracker.screen;
	
	if (visible)
	{
		control->setSize(PPSize(screen->getWidth(),
							tracker.MAXEDITORHEIGHT()-tracker.INSTRUMENTSECTIONDEFAULTHEIGHT()-tracker.UPPERSECTIONDEFAULTHEIGHT()));
	}
	else
	{
		control->setSize(PPSize(screen->getWidth(),tracker.MAXEDITORHEIGHT()-tracker.UPPERSECTIONDEFAULTHEIGHT()));
	}
}

pp_int32 SectionLink::handleEvent(PPObject* sender, PPEvent* event) 
{
	return 0;
}

void SectionLink::showSection(bool bShow) 
{
	containerEntire->show(bShow);
}

void SectionLink::init() 
{
	init(0, tracker.MAXEDITORHEIGHT()-tracker.INSTRUMENTSECTIONDEFAULTHEIGHT());
}

void SectionLink::init(pp_int32 x, pp_int32 y) {
	printf("initializing link section\n");
	PPScreen* screen = tracker.screen;

	pp_int32 screenW = screen->getWidth();
	pp_int32 screenH = screen->getHeight();

	containerEntire = new PPTransparentContainer
		(CONTAINER_ENTIRELINKSECTION,
		 screen,
		 this,
		 PPPoint(0, 0),
		 PPSize(screenW, screenH));

	printf("w: %d, h: %d, x: %d, y: %d", screenW, screenH, x, y);

	containerBottom = new PPContainer
		(CONTAINER_LINKINFO, 
		 screen,
		 this,
		 PPPoint(x, y),
		 PPSize(screenW - x, screenH - y),
		 false);
	containerEntire->addControl(containerBottom);

	// FROM HERE FORWARD, our coordinates will be relative to the beginning
	// of the Ableton Link UI

	PPStaticText *linkStatus = new PPStaticText(0, screen, NULL, PPPoint(x + 4, y + 4), "Ableton Link Status: ", true);
	containerBottom->addControl(linkStatus);

	containerBottom->setColor(TrackerConfig::colorThemeMain);

	containerEntire->adjustContainerSize();
	screen->addControl(containerEntire);
	initialised = true;
	showSection(false);
}

void SectionLink::update(bool repaint/* = true; */) {
	PPScreen *screen = tracker.screen;
	screen->paintControl(containerBottom, false);
	if (repaint)
		screen->update();
}

