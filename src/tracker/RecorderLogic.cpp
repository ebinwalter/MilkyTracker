/*
 *  tracker/PlayerLogic.cpp
 *
 *  Copyright 2009 Peter Barth
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  RecorderLogic.cpp
 *  MilkyTracker
 *
 *  Created by Peter Barth on 09.04.08.
 *
 */

#include "RecorderLogic.h"
#include "Tracker.h"
#include "TrackerConfig.h"
#include "PlayerController.h"
#include "PlayerLogic.h"
#include "PatternEditorControl.h"
#include "RecPosProvider.h"
#include "ModuleEditor.h"

#include "Screen.h"

void RecorderLogic::reset()
{
	memset(keys, 0, sizeof(TKeyInfo) * TrackerConfig::MAXNOTES);

	for (pp_int32 i = 0; i < TrackerConfig::MAXNOTES; i++)
		keys[i].channel = -1;
}

RecorderLogic::RecorderLogic(Tracker& tracker) :
	tracker(tracker),
	recordMode(false), recordKeyOff(true), recordNoteDelay(false)
{
	keys = new TKeyInfo[TrackerConfig::MAXNOTES];
	
	reset();
	
	keyVolume = -1;
}

RecorderLogic::~RecorderLogic()
{
	delete[] keys;
}

void RecorderLogic::sendNoteDownToPatternEditor(PPEvent* event, pp_int32 note, PatternEditorControl* patternEditorControl)
{
	PlayerController* playerController = tracker.playerController;
	PatternEditor* patternEditor = patternEditorControl->getPatternEditor();

	pp_int32 i;
		
	patternEditorControl->roundRobin();
	
	if (note >= 1 && note != PatternTools::getNoteOffNote() /* Key Off */)
	{
		
		// get current channel from pattern editor (= channel to play)
		pp_int32 chn = patternEditorControl->getCurrentChannel();	
		
		// if this is a valid note
		pp_int32 ins = tracker.getInstrumentToPlay(note, playerController);
		
		if (ins < 0)
		{
			if (event)
				event->cancel();
			return;
		}

		bool record = (tracker.editMode == EditModeMilkyTracker ? tracker.screen->hasFocus(patternEditorControl) : recordMode);
		bool releasePlay = false;
		bool isLiveRecording = playerController->isPlaying() && 
							!playerController->isPlayingRowOnly() &&
							record && 
							tracker.shouldFollowSong();
		// separated out releaseplay filtering from the currently in use isLiveRecording to break as few things as possible while filtering note key repeats from the OS
		// 
		// Key repeat suppression is enabled when we are looking at a note column (wider suppression than previously done with isLiveRecording)
		releasePlay = patternEditorControl->getCursorPosInner() == 0;

		if (releasePlay)
		{
			// Somewhere editing of text in an edit field takes place,
			// in that case we don't want to be playing anything
			if (tracker.isActiveEditing())
				return;
			
			// take a look if this key is already pressed
			bool isPressed = false;
			for (i = 0; i < TrackerConfig::MAXNOTES; i++)
			{
				if (keys[i].note == note)
				{
					isPressed = true;
					break;
				}
			}
			// this key is already pressed, we won't play that note again
			if (isPressed)
			{
				// If we're recording, this event should not be routed anywhere else
				// it terminates HERE! (event shall not be routed to the pattern editor)
				if (!playerController->isPlayingRowOnly() && record && event)
					event->cancel();
				return;
			}
			
			// if we're not recording, cycle through the channels
			// use jam-channels for playing if requested, 
			// use jam channels when recording/demoing notes under playback when not following song. 
			if (!(playerController->isPlaying() && record) || !tracker.shouldFollowSong())
			{
				chn = playerController->getNextPlayingChannel(chn);
			}
			else
			{
				// Get next recording channel: The base for selection of the 
				// next channel is the current channel within the pattern editor
				chn = playerController->getNextRecordingChannel(chn);
			}
		}
		else
		{
			// The cursor in the pattern editor must be located in the note column, 
			// if not abort
			if (patternEditorControl->getCursorPosInner() != 0)
			{
				//event->cancel();
				return;
			}
			else
			{
				pp_int32 newIns = patternEditor->getCurrentActiveInstrument();
				if (newIns >= 0)
					ins = newIns;
			}
		}

		RecPosProvider recPosProvider(*playerController, roundToClosestRow && !recordNoteDelay);
		// key is not pressed, play note and remember key + channel + position within module
		pp_int32 pos = -1, row = 0, ticker = 0;
		bool roundedRow = false;
		
		// if we are recording we are doing a query on the current position
		if (isLiveRecording)
			roundedRow = recPosProvider.getPosition(pos, row, ticker);
		else
		{
			pos = row = -1;
		}
		
		if (chn != -1)
		{

			// apply roundrobin if any
			PatternEditorControl::RoundRobin *rr = patternEditorControl->getRoundRobin();
			int currentIns = ins;
			ins = rr->ins_size > 0 ? (rr->ins_start + rr->ins_index) : ins;

			for (i = 0; i < TrackerConfig::MAXNOTES; i++)
			{
				// key not pressed or note already playing on this channel
				if (!keys[i].note || (keys[i].channel == chn))
				{
					keys[i].note = note;
					keys[i].ins = ins;
					keys[i].channel = chn;
					keys[i].pos = pos;
					keys[i].row = row;
					keys[i].playerController = playerController;
					break;
				}
				// if there is already a note playing on this channel
				// we "cut" the note
				//else if (keys[i].channel == chn)
				//{
				//	keys[i].note = keys[i].channel = 0;
				//}
			}

			
			// play it
			tracker.playerLogic->playNote(*playerController, (mp_ubyte)chn, note, 
										  ins, keyVolume);

			
			// if we're recording send the note to the pattern editor
			if (isLiveRecording)
			{
				tracker.setChanged();
				// update cursor to song position in case we're blocking refresh timer
				tracker.updateSongPosition(pos, row, true);
				pp_int32 posInner = patternEditorControl->getCursorPosInner();
				patternEditorControl->setChannel(chn, posInner);
				patternEditorControl->setRow(row);
				patternEditor->setCurrentInstrument(ins);
				// add delay note if requested
				if (ticker && recordNoteDelay)
					patternEditor->writeDirectEffect(1, 0x3D, ticker > 0xf ? 0xf : ticker,
													 chn, row, pos);
				else if (roundedRow)
					patternEditor->writeDirectEffect(1, 0x80, 0,
													 chn, row, pos);
				
				if (keyVolume != -1 && keyVolume >= 0 && keyVolume <= 255)
					patternEditor->writeDirectEffect(0, 0xC, (pp_uint8)keyVolume,
													 chn, row, pos);

				patternEditor->writeDirectNote(note, chn, row, pos);
				
				tracker.screen->paintControl(patternEditorControl);
				
				// update cursor to song position in case we're blocking refresh timer
				//updateSongPosition(-1, -1, true);
				
				if (event)
					event->cancel();
			}
			patternEditor->setCurrentInstrument(currentIns); // undo rr if any
		}
		else if (event)
		{
			event->cancel();
		}
		
	}
}

void RecorderLogic::sendNoteUpToPatternEditor(PPEvent* event, pp_int32 note, PatternEditorControl* patternEditorControl)
{
	// if this is a valid note look if we're playing something and release note by sending key-off
	if (note >= 1 && note <= ModuleEditor::MAX_NOTE)
	{	
		PatternEditor* patternEditor = patternEditorControl->getPatternEditor();

		pp_int32 pos = -1, row = 0, ticker = 0;
		
		bool record = (tracker.editMode == EditModeMilkyTracker ? tracker.screen->hasFocus(patternEditorControl) : recordMode);	

		for (mp_sint32 i = 0; i < TrackerConfig::MAXNOTES; i++)
		{
			// found a playing channel
			if (keys[i].note == note)
			{
				PlayerController* playerController = tracker.playerController;
				if (keys[i].playerController)
					playerController = keys[i].playerController;
			
				bool isLiveRecording = playerController->isPlaying() && 
									   !playerController->isPlayingRowOnly() &&
									   record && 
									   tracker.shouldFollowSong();
							   				
				bool recPat = false;
				RecPosProvider recPosProvider(*playerController, roundToClosestRow && !recordNoteDelay);
				if (isLiveRecording)
				{
					recPosProvider.getPosition(pos, row, ticker);
					if (pos == -1)
						recPat = true;
				}
				else
				{
					pos = row = -1;
				}
							
				// send key off
				tracker.playerLogic->playNote(*playerController, (mp_ubyte)keys[i].channel, 
											  PatternTools::getNoteOffNote(), 
											  keys[i].ins);
				
				if (isLiveRecording && recordKeyOff)
				{														
					tracker.setChanged();
					// update cursor to song position in case we're blocking refresh timer
					tracker.updateSongPosition(pos, row, true);
					patternEditorControl->setRow(row);

					// if we're in the same slot => send key off by inserting key off effect
					if (keys[i].row == row && keys[i].pos == pos)
					{
						// writing a note off to the same cell will overwrite the note on..
						// skip one row
						if (roundToClosestRow && !recordNoteDelay) {
							recPosProvider.incrementRow(pos, row);
							patternEditor->writeDirectNote(PatternTools::getNoteOffNote(),
														   keys[i].channel, row, pos);
						}
						//mp_sint32 bpm, speed;
						//playerController->getSpeed(bpm, speed);
						else
							patternEditor->writeDirectEffect(1, 0x14, ticker ? ticker : 1,
															 keys[i].channel, row, pos);
					}
					// else write key off
					else
					{
						if (ticker && recordNoteDelay)
							patternEditor->writeDirectEffect(1, 0x14, ticker,
															 keys[i].channel, row, pos);
						else
							patternEditor->writeDirectNote(PatternTools::getNoteOffNote(),
														   keys[i].channel, row, pos);
					}
				
					tracker.screen->paintControl(patternEditorControl);

					// update cursor to song position in case we're blocking refresh timer
					//updateSongPosition(-1, -1, true);
				}
				
				keys[i].note = 0;
				keys[i].channel = -1;
			}
		}
		
	}
}

void RecorderLogic::init()
{
	if (recordMode)
		tracker.playerController->initRecording();					
}

void RecorderLogic::initToggleEdit()
{
	if (recordMode && (tracker.playerController->isPlaying()))
		tracker.playerController->initRecording();							
}
