/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "gamemenu.h"

using namespace Chaos;

GameMenu::GameMenu() {
}

GameMenu* GameMenu::getInstance() {
  static GameMenu menu;    
  return &menu;
}

void GameMenu::sendSequence(Controller* controller) {
  // send sequence
  sequence.send(controller);
	
  // destory sequence
  sequence.clear();
}

void Menuing::selectRenderMode( int renderMode, Controller* dualshock ) {
	
  selectExtraItem( EXTRA_RENDER );	
  sequence.addTimeDelay(MENU_SELECT_DELAY);	// may another delay for render modes

  int difference = renderMode - render;
  moveToMenuItem( difference );
  sequence.addButtonPress( BUTTON_X );
  moveToMenuItem( -difference );
	
  deselectExtra();
	
  render = 0;//renderMode;	
  renderModesApplied++;
  
  sendSequence( dualshock );
}

void Menuing::teardownRenderMode( Controller* dualshock ) {
    renderModesApplied--;
    if (renderModesApplied == 0) {
        selectRenderMode( RENDER_NONE, dualshock );
        renderModesApplied = 0;
    }
}

void Menuing::teardownAudio( int whatToTeardown, Controller* dualshock ) {
  if (whatToTeardown == audioPitch) {
    selectGameplayMode(whatToTeardown, dualshock);
    audioPitch = AUDIO_DEFAULT;
  } else if (whatToTeardown == audioCrappifier) {
    selectGameplayMode(whatToTeardown, dualshock);
    audioCrappifier = AUDIO_DEFAULT;
  }
}

void Menuing::selectGameplayMode( int gameplayMode, Controller* dualshock ) {
    
  selectExtraItem( EXTRA_GAMEPLAY );
	
  int difference = gameplayMode - gameplay;
  moveToMenuItem( difference );
  sequence.addButtonPress( BUTTON_X );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
	
  // reset:
  moveToMenuItem( -difference );

  deselectExtra();
	
  gameplay = 0;//gameplayMode;
    
  sendSequence( dualshock );
	
  if (gameplayMode == GAMEPLAY_AUDIO_XENON ||
      gameplayMode == GAMEPLAY_AUDIO_HELIUM) {
    audioPitch = gameplayMode;
  } else if (
	     gameplayMode == GAMEPLAY_AUDIO_FOUR_BIT ||
	     gameplayMode == GAMEPLAY_AUDIO_EIGHT_BIT ) {
    audioCrappifier = gameplayMode;
  }
}

void Menuing::selectHudMode( int hudOption, int option, Controller* dualshock ) {
    // build sequence
    selectHudItem( hudOption );    
    if (hudOption == HUD_RETICLE) {
      sequence.addAxisPress( AXIS_DX, -1);
      sequence.addAxisPress( AXIS_DX, -1);
      for (int i = 0; i < option; i++) {
	sequence.addAxisPress( AXIS_DX, 1);
      }
    } else {
      for (int i = 0; i < option; i++) {
	sequence.addAxisPress( AXIS_DX, 1);
      }
      for (int i = 0; i > option; i--) {
	sequence.addAxisPress( AXIS_DX, -1);
      }
    }
    deselectHud();
    sendSequence( dualshock );
}

void Menuing::selectSubtitleMode( int subtitleOption, int option, Controller* dualshock ) {
  selectSubtitleItem( subtitleOption );
	
  for (int i = 0; i < option; i++) {
    sequence.addAxisPress( AXIS_DX, 1);
  }
  for (int i = 0; i > option; i--) {
    sequence.addAxisPress( AXIS_DX, -1);
  }
  deselectSubtitle();
  sendSequence( dualshock );
}

void Menuing::setDisplayMode( int displayOption, bool turnOn, Chaos::Controller* dualshock ) {
  selectDisplayItem( displayOption );
	
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
  } else {
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  deselectDisplay();	
  sendSequence( dualshock );
}

void Menuing::setVolumeMode( int volumeOption, int amount, Chaos::Controller* dualshock ) {
  selectAudioItem( volumeOption );
	
  for (int i = 0; i < amount; i++) {
    sequence.addAxisPress( AXIS_DX, 1);
  }
  for (int i = 0; i > amount; i--) {
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  deselectAudio();	
  sendSequence( dualshock );
}

void Menuing::moveToMenuItem( int difference ) {
  for (int i = 0; i < difference; i++) {
    sequence.addAxisPress( AXIS_DY, 1);
  }
  for (int i = 0; i > difference; i--) {
    sequence.addAxisPress( AXIS_DY, -1);
  }
}

void Menuing::enterMainMenu() {
  sequence.disablejoysticks();
  sequence.addButtonPress( BUTTON_OPTIONS );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
}

void Menuing::selectMainMenuItem( int selection ) {
  enterMainMenu();
	
  int difference = selection - mainMenu + ( (numberAudioCuesEnabled > 0  && selection >= MENU_EXTRAS) ? 1 : 0 );
  moveToMenuItem( difference );
  sequence.addButtonPress( BUTTON_X );
	
  sequence.addTimeDelay(MENU_SELECT_DELAY);
    
  mainMenu = MENU_OPTIONS;// selection;
}

void Menuing::selectExtraItem( int selection ) {
  selectMainMenuItem( MENU_EXTRAS );
	
  int difference = selection - extras;
  moveToMenuItem( difference );
  sequence.addButtonPress( BUTTON_X );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
    
  extras = selection;
}

void Menuing::selectOptionItem( int selection ) {
  selectMainMenuItem( MENU_OPTIONS );
	
  int difference = selection - options;
  moveToMenuItem( difference );
  sequence.addButtonPress( BUTTON_X );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
    
  options = selection;
}

void Menuing::selectHudItem( int selection ) {
  selectOptionItem( OPTIONS_HUD );
	
  int difference = selection - hud;
  moveToMenuItem( difference );
//    sequence.addButtonPress( BUTTON_X );
    
  hud = selection;    
}

void Menuing::selectSubtitleItem( int selection ) {
  selectOptionItem( OPTIONS_SUBTITLES );

  sequence.addAxisPress( AXIS_DX, 1);
  sequence.addAxisPress( AXIS_DX, 1);
	
  int difference = selection - subtitle;
  moveToMenuItem( difference );
  subtitle = selection;	
}

void Menuing::selectDisplayItem( int selection ) {
  selectOptionItem( OPTIONS_DISPLAY );
	
  int difference = selection - display;
  moveToMenuItem( difference );
  display = selection;
}

void Menuing::selectAudioItem( int selection ) {
  selectOptionItem( OPTIONS_AUDIO );
	
  int difference = selection - audio;
  moveToMenuItem( difference );
  audio = selection;
}

void Menuing::selectAccessibilityItem( int item ) {
  selectOptionItem( OPTIONS_ACCESSIBILITY );
  moveToMenuItem(item - accessibility);
  sequence.addButtonPress( BUTTON_X ); 
  sequence.addTimeDelay(MENU_SELECT_DELAY);
  accessibility = item;
}

void Menuing::setMagnificationAndAids( int item, bool turnOn, Controller* dualshock ) {
  selectAccessibilityItem( ACCESSIBILITY_MAGNIFICATION_AND_VISUAL_AIDS );
  moveToMenuItem( item - magnification );
	
  // Right now only HCD and Scale are supported, so only one press is needed
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
  } else {
    sequence.addAxisPress( AXIS_DX, -1);
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  moveToMenuItem( -(item - magnification) );
  deselectAccessibility();
  //sequence.addButtonPress( BUTTON_OPTIONS );	// exit
	
  this->magnification = 0;//item;
	
  sendSequence( dualshock );
}

void Menuing::setAlternateControls( int item, bool turnOn, Controller* dualshock ) {
  selectAccessibilityItem( ACCESSIBILITY_ALTERNATE_CONTROLS );
  moveToMenuItem( item - alternateControls );
	
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
    sequence.addAxisPress( AXIS_DX, 1);
  } else {
    sequence.addAxisPress( AXIS_DX, -1);
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  moveToMenuItem( -(item - alternateControls) );
  deselectAccessibility();
//	sequence.addButtonPress( BUTTON_OPTIONS );	// exit
	
  this->alternateControls = 0;//item;
	
  sendSequence( dualshock );
}

void Menuing::setNavigationAndTraversal( int item, bool turnOn, Controller* dualshock ) {
  selectAccessibilityItem( ACCESSIBILITY_NAVIGATION_AND_TRAVERSAL );
	
  moveToMenuItem( item - navigation );
	
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
  } else {
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  moveToMenuItem( -(item - navigation) );
  deselectAccessibility();
  //sequence.addButtonPress( BUTTON_OPTIONS );	// exit
	
  this->navigation = 0;//item;
	
  sendSequence( dualshock );
	
}

void Menuing::setTtsAndAudioCues( int item, bool turnOn, Controller* dualshock ) {
  selectAccessibilityItem( ACCESSIBILITY_TTS_AND_AUDIO_CUES );
	
  moveToMenuItem( item - tts );
	
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
    numberAudioCuesEnabled++;
  } else {
    numberAudioCuesEnabled--;
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  moveToMenuItem( -(item - tts) );
  deselectAccessibility();
  //sequence.addButtonPress( BUTTON_OPTIONS );	// exit
	
  this->tts = 0;//item;
	
  sendSequence( dualshock );
	
}

void Menuing::setCombatAccessibility( int item, bool turnOn, Controller* dualshock ) {
  selectAccessibilityItem( ACCESSIBILITY_COMBAT_ACCESSIBILITY );
	
  // Unique for this is to ensure that they are turned on.
  sequence.addAxisPress( AXIS_DX, 1);
	
  moveToMenuItem( item - combat );
	
  // All combat accessibilities have either 1 or 2 options, pressing right enables them
  if (turnOn) {
    sequence.addAxisPress( AXIS_DX, 1);
    sequence.addAxisPress( AXIS_DX, 1);
  } else {
    sequence.addAxisPress( AXIS_DX, -1);
    sequence.addAxisPress( AXIS_DX, -1);
  }
	
  //this->combat = item;
  moveToMenuItem( -(item - combat) );
  deselectAccessibility();
  //sequence.addButtonPress( BUTTON_OPTIONS );	// exit
	
  this->combat = 0;
	
  sendSequence( dualshock );
	
}

void Menuing::exitMainMenu() {
  sequence.addTimeDelay(MENU_SELECT_DELAY);
  sequence.addButtonPress( BUTTON_OPTIONS );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
}

void Menuing::deselectMainMenu() {
  sequence.addTimeDelay(MENU_SELECT_DELAY);
  exitMainMenu();
}

void Menuing::deselectExtra() {
  sequence.addButtonPress( BUTTON_CIRCLE );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
	
  int selection = 0;
  int difference = selection - extras;
  moveToMenuItem( difference );
	
  deselectMainMenu();
	
  extras = selection;
}

void Menuing::deselectOption() {
  sequence.addButtonPress( BUTTON_CIRCLE );
  sequence.addTimeDelay(MENU_SELECT_DELAY);
	
  int selection = 0;
  moveToMenuItem( selection - options );
  options = selection;
  deselectMainMenu( );
}

void Menuing::deselectHud() {
  int selection = 0;
  moveToMenuItem( selection - hud );
  hud = selection;
	
  deselectOption();	
}

void Menuing::deselectSubtitle() {
  int selection = 0;
  moveToMenuItem( selection - subtitle );
  subtitle = selection;	
  deselectOption();
}

void Menuing::deselectDisplay() {
  int selection = 0;
  moveToMenuItem( selection - display );
  display = selection;
  deselectOption();
}

void Menuing::deselectAudio() {
  int selection = 0;
  moveToMenuItem( selection - audio );
  audio = selection;	
  deselectOption();
}

void Menuing::deselectAccessibility() {
  sequence.addButtonPress(BUTTON_CIRCLE);
  sequence.addTimeDelay(MENU_SELECT_DELAY);
	
  int item = 0;
  moveToMenuItem(item - accessibility);
  accessibility = item;	
  deselectOption();
}