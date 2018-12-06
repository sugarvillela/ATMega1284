/****************************************************************************
    This file is part of "Midi Record/Play/Overdub With 5-Pin Connections", 
	"MRecord" for short, Copyright 2018, Dave S. Swanson.

    MRecord is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRecord is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRecord.  If not, see <https://www.gnu.org/licenses/>.
*****************************************************************************/

#include <avr/io.h>
#include "menu.h"
#include "buttonbus.h"
#include "lcddual.h"
#include "transport.h"
#include "userin.h"
#include "midiin.h"
#include "song.h"
#include "UT.h"

/*	This is currently implemented as a two-D linked list 
	It would be a little cheaper to do it as an array 
	Now each struct has 4 pointers.  If I replace the pointers with uchar array indexes
	each struct will be 12 bytes smaller, assuming pointers are 4 bytes 
	
	Note indenting style to illustrate menu structure
*/

void menuUp();
void menuDown();
void menuLeft();
void menuRight();
void menuLeftRoot();

/* Utility scripts */
void verboseScript(){
	/*	This gives user a chance to take finger off button...really 
		Otherwise it jumps right back to where it was */
	menuLeftRoot();
	msgBindDropEvent( 1, &menuOn );
	msgWriteErr( 1, "Saved!", MSG_SHORT );
}
void tempoScript(){
	//uchar newTempo=numinGetVal();			//get tempo from numin
	//char buf[5];
	//nToChars( newTempo, buf );
	//msgWriteErr( 0, buf, MSG_LONG );
	//
	//nToChars( getTempo(), buf );
	//msgBindDropEvent( 1, &menuOn );
	//msgWriteErr( 1, buf, MSG_LONG );
	writeNewTempo(  getTempo(), numinGetVal() );//get value from transport; change timestamps in midiin   getTempo(), newTempo 
	setTempo( numinGetVal() );					//update in transport
	menuLeftRoot();
	menuWrite();
}
void tapScript(){
	uchar newTempo=tapGetVal();				//get tempo from tap
	writeNewTempo( getTempo(), newTempo );	//get value from transport; change timestamps in midiin
	setTempo( newTempo );					//update in transport
	menuLeftRoot();
	menuWrite();
}

/*	Value functions; not all menu items need to display a current value so 
	some menu items have null for value function pointer
*/
void value_m_0_song( uchar buf[] ){ song_getTitle( (char*)buf ); }//song name
	void value_m_1_pResume( uchar buf[] ){ getTimeMB( buf ); }// play from current time
	void value_m_1_playTop( uchar buf[] ){ cp( (char*)buf, "1:1" ); }
	void value_m_1_rResume( uchar buf[] ){ getTimeMB( buf ); }// current measure:beat from transport
	void value_m_1_recTop( uchar buf[] ){ cp( (char*)buf, "1:1" ); }
	void value_m_1_tempo( uchar buf[] ){ getTempo_str( buf ); }//tempo from transport
	void value_m_1_tsig( uchar buf[] ){ getTimeSig_str( buf ); }//tsig from transport
	void value_m_1_event( uchar buf[] ){ eventStatus( buf ); }//number of events, from midiin
	void value_m_1_metOn( uchar buf[] ){ onOff( getBeepOn(), buf ); }//soundOn=1 from transport
	void value_m_1_dispMS( uchar buf[] ){ onOff( getDispMSOn(), buf ); }//M:B time or raw time=1

/*	Action functions; Only leaf items have actions */
       
void go_m_1_new(){
	transportInit();
	song_init();
	midiInit();
	msgBindDropEvent( 0, &transportOn );
	msgWriteErr( 0, "Preparing...", MSG_SHORT );
}
void go_m_1_saveAs(){
	uinBindSaveEvent( &verboseScript );
	uinOn();
}
void go_m_1_save(){
	if( song_isTitled() ){
		song_serialize();
	}
	else{
		go_m_1_saveAs();
	}
}
void go_m_1_open(){
	transportInit();
	msgBindDropEvent( 0, &transportOn );
	msgWriteErr( 0, "Opening...", MSG_SHORT );
	song_load();
}
void go_m_1_pResume(){ 
	transportOn(); 
	play(); 
}
void go_m_1_playTop(){
	transportTop();
	transportOn();
	play();
}
void go_m_1_rResume(){
	transportOn();
	record();
}
void go_m_1_recTop(){
	transportTop();
	transportOn();
	record();
}
	void go_m_2_tempoSet(){
		numinBindSaveEvent( &tempoScript );
		numinOn();
	}
	void go_m_2_tempoTap(){
		tapBindSaveEvent( &tapScript );
		tapOn();
	}
	void go_m_2_tsig44(){ 
		setTimeSignature( FOUR_FOUR ); 
		menuLeftRoot();
		menuWrite();
	}
	void go_m_2_tsig34(){ 
		setTimeSignature( THREE_FOUR ); 
		menuLeftRoot();
		menuWrite(); 
	}
	void go_m_2_tsig68(){ 
		setTimeSignature( SIX_EIGHT ); 
		menuLeftRoot();
		menuWrite();
	}
void go_m_1_event(){
	if( haveEvents()){ 
		menuLeftRoot();
		midiBindOffEvent( &menuOn );
		eventDispOn();
	}
}
	void go_m_2_q14(){ 
		quantize( getTempo(), getTsig2(), 4 );
		menuLeftRoot();
		menuWrite();
		//if( haveEvents()){
			//quantize( getTempo(), getTsig2(), 4 );
			//menuLeftRoot();
			//menuWrite();
		//}
	}
	void go_m_2_q18(){
		quantize( getTempo(), getTsig2(), 8 );
		menuLeftRoot();
		menuWrite();
		//if( haveEvents()){
			//quantize( getTempo(), getTsig2(), 8 );
			//menuLeftRoot();
			//menuWrite();
		//}
	}
	void go_m_2_q116(){
		quantize( getTempo(), getTsig2(), 16 );
		menuLeftRoot();
		menuWrite();
		//if( haveEvents()){
			//quantize( getTempo(), getTsig2(), 16 );
			//menuLeftRoot();
			//menuWrite();
		//}
	}
void go_m_1_metOn(){
	toggleBeepOn();
	menuLeftRoot();
	menuWrite();
}
void go_m_1_dispMS(){
	toggleDispMS();
	menuLeftRoot();
	menuWrite();
}
/* Using pointers without malloc: each pointer needs a struct to point to */
mStruct
	m_0_song,
		m_1_new,
		m_1_save,
		m_1_saveAs,
		m_1_open,
	m_0_play,//later
		m_1_pResume,
		m_1_playTop,
	m_0_rec,
		m_1_rResume,
		m_1_recTop,
	m_0_time,
		m_1_tempo,
			m_2_tempoSet,//later
			m_2_tempoTap,
		m_1_tsig,
			m_2_tsig44,
			m_2_tsig34,
			m_2_tsig68,
	m_0_proc,
		m_1_event,
		m_1_quant,
			m_2_q14,
			m_2_q18,
			m_2_q116,
	m_0_opt,
		m_1_metOn,
		m_1_dispMS;

mStruct* LEFT;                      //current left screen
mStruct* RIGHT;                     //current right screen

/* Navigation functions bound to button-bus items */
void menuUp(){
    if( !LEFT->up ){
        return;
    }
    LEFT=LEFT->up;
    RIGHT=LEFT->right;
    menuWrite();
}
void menuDown(){
    if( !LEFT->down ){
        return;
    }
    LEFT=LEFT->down;
    RIGHT=LEFT->right;
    menuWrite();
}
void menuLeft(){
    if( !LEFT->left ){
        return;
    }
    LEFT=LEFT->left;
    RIGHT=LEFT->right;
    menuWrite();
}
void menuRight(){
    if( !LEFT->right ){
        LEFT->go();
        return;
    }
    LEFT=LEFT->right;
    RIGHT=LEFT->right;
    menuWrite();
}
void menuLeftRoot(){
	while( LEFT->left ){
		LEFT=LEFT->left;
		RIGHT=LEFT->right;
	}
	//menuWrite();
}
void menuInit(){
    /* initialize layers */
    cp( m_0_song.name, " Song" );
    m_0_song.value=&value_m_0_song;
    m_0_song.go=0;
    m_0_song.left=0;
    m_0_song.up=0;
    m_0_song.down=&m_0_play;
    m_0_song.right=&m_1_new;

		cp( m_1_new.name, " New" );
		m_1_new.value=0;
		m_1_new.go=&go_m_1_new;
		m_1_new.left=&m_0_song;
		m_1_new.up=0;
		m_1_new.down=&m_1_save;
		m_1_new.right=0;
    
		cp( m_1_save.name, " Save" );
		m_1_save.value=0;
		m_1_save.go=&go_m_1_save;
		m_1_save.left=&m_0_song;
		m_1_save.up=&m_1_new;
		m_1_save.down=&m_1_saveAs;
		m_1_save.right=0;
    
		cp( m_1_saveAs.name, " SaveAs" );
		m_1_saveAs.value=0;
		m_1_saveAs.go=&go_m_1_saveAs;
		m_1_saveAs.left=&m_0_song;
		m_1_saveAs.up=&m_1_save;
		m_1_saveAs.down=&m_1_open;
		m_1_saveAs.right=0;
		
		cp( m_1_open.name, " Open" );
		m_1_open.value=0;
		m_1_open.go=&go_m_1_open;
		m_1_open.left=&m_0_song;
		m_1_open.up=&m_1_saveAs;
		m_1_open.down=0;
		m_1_open.right=0;
    
    cp( m_0_play.name, " Play" );
    m_0_play.value=0;
    m_0_play.go=0;
    m_0_play.left=0;
    m_0_play.up=&m_0_song;
    m_0_play.down=&m_0_rec;
    m_0_play.right=&m_1_pResume;
    
		cp( m_1_pResume.name, " Resume" );
		m_1_pResume.value=&value_m_1_pResume;
		m_1_pResume.go=&go_m_1_pResume;
		m_1_pResume.left=&m_0_play;
		m_1_pResume.up=0;
		m_1_pResume.down=&m_1_playTop;
		m_1_pResume.right=0;
    
		cp( m_1_playTop.name, " Top" );
		m_1_playTop.value=&value_m_1_playTop;
		m_1_playTop.go=&go_m_1_playTop;
		m_1_playTop.left=&m_0_play;
		m_1_playTop.up=&m_1_pResume;
		m_1_playTop.down=0;
		m_1_playTop.right=0;
    
    cp( m_0_rec.name, " Record" );
    m_0_rec.value=0;
    m_0_rec.go=0;
    m_0_rec.left=0;
    m_0_rec.up=&m_0_play;
    m_0_rec.down=&m_0_time;
    m_0_rec.right=&m_1_rResume;

		cp( m_1_rResume.name, " Resume" );
		m_1_rResume.value=&value_m_1_rResume;
		m_1_rResume.go=&go_m_1_rResume;
		m_1_rResume.left=&m_0_rec;
		m_1_rResume.up=0;
		m_1_rResume.down=&m_1_recTop;
		m_1_rResume.right=0;
    
		cp( m_1_recTop.name, " Top" );
		m_1_recTop.value=&value_m_1_recTop;
		m_1_recTop.go=&go_m_1_recTop;
		m_1_recTop.left=&m_0_rec;
		m_1_recTop.up=&m_1_rResume;
		m_1_recTop.down=0;
		m_1_recTop.right=0;
    
    cp( m_0_time.name, " Time" );
    m_0_time.value=0;
    m_0_time.go=0;
    m_0_time.left=0;
    m_0_time.up=&m_0_rec;
    m_0_time.down=&m_0_proc;
    m_0_time.right=&m_1_tempo;

		cp( m_1_tempo.name, " Tempo" );
		m_1_tempo.value=&value_m_1_tempo;
		m_1_tempo.go=0;
		m_1_tempo.left=&m_0_time;
		m_1_tempo.up=0;
		m_1_tempo.down=&m_1_tsig;
		m_1_tempo.right=&m_2_tempoSet;

			cp( m_2_tempoSet.name, " Set" );
			m_2_tempoSet.value=0;
			m_2_tempoSet.go=&go_m_2_tempoSet;
			m_2_tempoSet.left=&m_1_tempo;
			m_2_tempoSet.up=0;
			m_2_tempoSet.down=&m_2_tempoTap;
			m_2_tempoSet.right=0;
		
			cp( m_2_tempoTap.name, " Tap" );
			m_2_tempoTap.value=0;
			m_2_tempoTap.go=&go_m_2_tempoTap;
			m_2_tempoTap.left=&m_1_tempo;
			m_2_tempoTap.up=&m_2_tempoSet;
			m_2_tempoTap.down=0;
			m_2_tempoTap.right=0;
			
		cp( m_1_tsig.name, " TSig" );
		m_1_tsig.value=&value_m_1_tsig;
		m_1_tsig.go=0;
		m_1_tsig.left=&m_0_time;
		m_1_tsig.up=&m_1_tempo;
		m_1_tsig.down=0;
		m_1_tsig.right=&m_2_tsig44;
		
			cp( m_2_tsig44.name, " 4:4" );
			m_2_tsig44.value=0;
			m_2_tsig44.go=&go_m_2_tsig44;
			m_2_tsig44.left=&m_1_tsig;
			m_2_tsig44.up=0;
			m_2_tsig44.down=&m_2_tsig34;
			m_2_tsig44.right=0;
			
			cp( m_2_tsig34.name, " 3:4" );
			m_2_tsig34.value=0;
			m_2_tsig34.go=&go_m_2_tsig34;
			m_2_tsig34.left=&m_1_tsig;
			m_2_tsig34.up=&m_2_tsig44;
			m_2_tsig34.down=&m_2_tsig68;
			m_2_tsig34.right=0;
			
			cp( m_2_tsig68.name, " 6:8" );
			m_2_tsig68.value=0;
			m_2_tsig68.go=&go_m_2_tsig68;
			m_2_tsig68.left=&m_1_tsig;
			m_2_tsig68.up=&m_2_tsig34;
			m_2_tsig68.down=0;
			m_2_tsig68.right=0;
    
    cp( m_0_proc.name, " Process" );
    m_0_proc.value=0;
    m_0_proc.go=0;
    m_0_proc.left=0;
    m_0_proc.up=&m_0_time;
    m_0_proc.down=&m_0_opt;
    m_0_proc.right=&m_1_event;

		cp( m_1_event.name, " Events" );
		m_1_event.value=&value_m_1_event;
		m_1_event.go=&go_m_1_event;
		m_1_event.left=&m_0_proc;
		m_1_event.up=0;
		m_1_event.down=&m_1_quant;
		m_1_event.right=0;
		
		cp( m_1_quant.name, " Quant" );
		m_1_quant.value=0;
		m_1_quant.go=0;
		m_1_quant.left=&m_0_proc;
		m_1_quant.up=&m_1_event;
		m_1_quant.down=0;
		m_1_quant.right=&m_2_q14;
		
			cp( m_2_q14.name, " 1/4" );
			m_2_q14.value=0;
			m_2_q14.go=&go_m_2_q14;
			m_2_q14.left=&m_1_quant;
			m_2_q14.up=0;
			m_2_q14.down=&m_2_q18;
			m_2_q14.right=0;
			
			cp( m_2_q18.name, " 1/8" );
			m_2_q18.value=0;
			m_2_q18.go=&go_m_2_q18;
			m_2_q18.left=&m_1_quant;
			m_2_q18.up=&m_2_q14;
			m_2_q18.down=&m_2_q116;
			m_2_q18.right=0;
			
			cp( m_2_q116.name, " 1/16" );
			m_2_q116.value=0;
			m_2_q116.go=&go_m_2_q116;
			m_2_q116.left=&m_1_quant;
			m_2_q116.up=&m_2_q18;
			m_2_q116.down=0;
			m_2_q116.right=0;
    
    cp( m_0_opt.name, " Option" );
    m_0_opt.value=0;
    m_0_opt.go=0;
    m_0_opt.left=0;
    m_0_opt.up=&m_0_proc;
    m_0_opt.down=0;
    m_0_opt.right=&m_1_metOn;

		cp( m_1_metOn.name, " Metro" );
		m_1_metOn.value=&value_m_1_metOn;
		m_1_metOn.go=&go_m_1_metOn;
		m_1_metOn.left=&m_0_opt;
		m_1_metOn.up=0;
		m_1_metOn.down=&m_1_dispMS;
		m_1_metOn.right=0;
		
		cp( m_1_dispMS.name, " M:B" );
		m_1_dispMS.value=&value_m_1_dispMS;
		m_1_dispMS.go=&go_m_1_dispMS;
		m_1_dispMS.left=&m_0_opt;
		m_1_dispMS.up=&m_1_metOn;
		m_1_dispMS.down=0;
		m_1_dispMS.right=0;
		
    /* Set up screen pointers */
    LEFT=&m_0_song;
    RIGHT=LEFT->right;
}
void menuOn(){
	bBusInit();
	msgUnbindDropEvent( 1 );//sometimes set by save actions
	msgQueueOff( 0 );
	msgQueueOff( 1 );
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
    bindPressEvent( 0, &menuLeft );
    bindPressEvent( 1, &menuUp );
    bindPressEvent( 2, &menuDown );
    bindPressEvent( 3, &menuRight );
    menuWrite();
}
void menuOff(){
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
}

void menuWrite(){
	LCD_ClearScreen( 0 );
    uchar val[9];
    if( LEFT ){
		LEFT->name[0]='>';
		LCD_Disp( 1, (const uchar*)LEFT->name, 0 );
		if( LEFT->value ){
			LEFT->value( val );
			LCD_Disp( 9, (const uchar*)val, 0 );
		}
		
		if( LEFT->down ){
			LEFT->down->name[0]=' ';
			LCD_Disp( 17, (const uchar*)LEFT->down->name, 0 );
			if( LEFT->down->value ){
				LEFT->down->value( val );
				LCD_Disp( 25, (const uchar*)val, 0 );
			}
		}
		LCD_Cursor( 1, 0 );
    }
	LCD_ClearScreen( 1 );
    if( RIGHT ){
		RIGHT->name[0]='>';
	    LCD_Disp( 1, (const uchar*)RIGHT->name, 1 );
	    if( RIGHT->value ){
		    RIGHT->value( val );
		    LCD_Disp( 9, (const uchar*)val, 1 );
	    }
	    if( RIGHT->down ){
			RIGHT->down->name[0]=' ';
		    LCD_Disp( 17, (const uchar*)RIGHT->down->name, 1 );
		    if( RIGHT->down->value ){
			    RIGHT->down->value( val );
			    LCD_Disp( 25, (const uchar*)val, 1 );
		    }
	    }
		LCD_Cursor( 1, 1 );
    }
}
