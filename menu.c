//#include <stdio.h>
#include <avr/io.h>
#include "menu.h"
#include "buttonbus.h"
#include "lcddual.h"
#include "transport.h"
#include "userin.h"
#include "UT.h"

/*	This is currently implemented as a two-D linked list 
	It would be a little cheaper to do it as an array 
	Now each struct has 4 pointers.  If I replace the pointers with uchar array indexes
	each struct will be 12 bytes smaller, assuming pointers are 4 bytes 
	
	Note indenting style to indicate menu structure
*/
void menuUp();
void menuDown();
void menuLeft();
void menuRight();

/*	Value functions; not all menu items need to display a current value so some of these 
	will be deleted 
*/
void value_m_0_song( uchar buf[] ){ cp( (char*)buf, "Untitled" ); }//song name
	void value_m_1_save( uchar buf[] ){ cp( (char*)buf, "Untitled" ); }//song name
	void value_m_1_pResume( uchar buf[] ){ getTimeMB( buf ); }// play from current time
	void value_m_1_playTop( uchar buf[] ){ cp( (char*)buf, "1:1" ); }
	void value_m_1_rResume( uchar buf[] ){ getTimeMB( buf ); }// rec from current time
	void value_m_1_recTop( uchar buf[] ){ cp( (char*)buf, "1:1" ); }
	void value_m_1_tempo( uchar buf[] ){ getTempo_str( buf ); }//tempo from metronome
	void value_m_1_tsig( uchar buf[] ){ getTimeSig_str( buf ); }//tsig from metronome	done
	void value_m_1_quant( uchar buf[] ){ cp( (char*)buf, "1/4" ); }
	void value_m_1_metOn( uchar buf[] ){ onOff( getBeepOn(), buf ); }//soundOn=1 from metronome
	void value_m_1_array( uchar buf[] ){ onOff( 0, buf ); }//LEDShow=1 from letMatrix

/*	Action functions; Only leaf items have actions */
       
void go_m_1_new(){LCD_DispNew( 1, (const uchar*)"go_new", 1 );}
void go_m_1_save(){
	//uinOn();
	LCD_DispNew( 1, (const uchar*)"go_save", 1 );
}
void go_m_1_saveAs(){
	uinOn();
	//LCD_DispNew( 1, (const uchar*)"go_saveAs", 1 );
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
		numinOn();
		//LCD_DispNew( 1, (const uchar*)"go_tempoSet", 1 );
	}
	void go_m_2_tempoTap(){
		tapOn();
		//LCD_DispNew( 1, (const uchar*)"go_tempoTap", 1 );
	}
//void go_m_1_tsig(){LCD_DispNew( 1, (const uchar*)"go_", 1 );}
	void go_m_2_tsig44(){ 
		setTimeSignature( FOUR_FOUR ); 
		menuLeft(); 
	}
	void go_m_2_tsig34(){ 
		setTimeSignature( THREE_FOUR ); 
		menuLeft(); 
	}
	void go_m_2_tsig68(){ 
		setTimeSignature( SIX_EIGHT ); 
		menuLeft(); 
	}
void go_m_1_quant(){LCD_DispNew( 1, (const uchar*)"go_quant", 1 );}
	void go_m_2_q14(){LCD_DispNew( 1, (const uchar*)"go_q14", 1 );}
	void go_m_2_q18(){LCD_DispNew( 1, (const uchar*)"go_q18", 1 );}
	void go_m_2_q116(){LCD_DispNew( 1, (const uchar*)"go_q116", 1 );}
void go_m_1_metOn(){
	toggleBeepOn();
	menuLeft();
}
void go_m_1_array(){LCD_DispNew( 1, (const uchar*)"go_LED", 1 );}

/* Using pointers without malloc: each pointer needs a struct to point to */
mStruct
	m_0_song,//later
		m_1_new,
		m_1_save,
		m_1_saveAs,
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
		m_1_quant,
			m_2_q14,
			m_2_q18,
			m_2_q116,
	m_0_opt,
		m_1_metOn,
		m_1_array;

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
		m_1_save.value=&value_m_1_save;
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
		m_1_saveAs.down=0;
		m_1_saveAs.right=0;
    
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
    m_0_proc.right=&m_1_quant;

		cp( m_1_quant.name, " Quant" );
		m_1_quant.value=&value_m_1_quant;
		m_1_quant.go=0;
		m_1_quant.left=&m_0_proc;
		m_1_quant.up=0;
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
		m_1_metOn.down=&m_1_array;
		m_1_metOn.right=0;
    
		cp( m_1_array.name, " Lights" );
		m_1_array.value=&value_m_1_array;
		m_1_array.go=&go_m_1_array;
		m_1_array.left=&m_0_opt;
		m_1_array.up=&m_1_metOn;
		m_1_array.down=0;
		m_1_array.right=0;
		
    /* Set up screen pointers */
    LEFT=&m_0_song;
    RIGHT=LEFT->right;
}
void menuOn(){
	bBusInit();
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
