#include "global.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "playerTask.h"
#include "debugTask.h"
#include "main.h"
#include "global.h"

void Background_Board(void);
void PAGE_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str);
void PAGE_Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h, char *str);
void viwePalitra(void);
void UI_List_Mp3();



int selectIndex = 0;
int encoder_block = 0;

int max_window_item = 6;
int window_start    = 0;
int window_end;
int NUM;


void task_HMI(void)
{

  NUM = list_mp3.maxFileCount;
  window_end = (NUM >= max_window_item) ? max_window_item : NUM;

  while(1)
  {
	KEY.tick();
	Background_Board();

	UI_List_Mp3();


	char str[64];
	sprintf(str, "%dk", list_mp3.size[selectIndex]/1024);

	tft.SetColor(3);
	gfxfont.Puts(10, 229, str, 16);

	tft.SetColor(5);
	sprintf(str, "G:%d", (int)(playerInfo.gain * 10.0F));
	gfxfont.Puts(177, 229, str, 16);

	tft.SetColor(5);
	sprintf(str, "E:%d", playerInfo.error);
	gfxfont.Puts(100, 229, str, 16);



	tft.LineH(154, 7, 232, 6 );
	tft.LineH(155, 7, 232, 2 );

	tft.LineH(209, 7, 232, 6 );
	tft.LineH(210, 7, 232, 2 );

	tft.RectangleFilled(9, 187, 221 , 20, 15);
	tft.RectangleFilled(11, 189, 217*playerInfo.fpersent , 16, 14);

	tft.SetColor(13);
	sprintf(str, "%lu",  playerInfo.timeCurrent);
	gfxfont.Puts(90, 205, str, 16);

	tft.SetColor(14);
	sprintf(str, "%s",  playerInfo.filename.c_str());
	gfxfont.Puts(10, 180, gfxfont.utf8rus2(str), 16);










	//play((char*)playerInfo.filename);
	//tft.RectangleFilled(0, 160, 200 * playerInfo.fpersent, 20, 14);

	//gfxfont.Puts(20, 120, gfxfont.utf8rus2("Пр1"));

	//viwePalitra();
	//PAGE_Button(10, 10, 50, 30,	1, (char*)"123");

	//PAGE_Button(10, 100, 50, 30,	0, (char*)"123");

	//PAGE_Button_Pressed(10, 150, 50, 30, (char*)"123");

    tft.Update();
  }
}

void viwePalitra(void)
{
	int color = 0;
	for(int y = 0; y < 4; y++)
		for(int x = 0; x < 4; x++)
		{
			tft.RectangleFilled(x*60, y*60 , 59, 59, color); //Основной фон
			char str[8];
			sprintf(str, "%d", color);
			gfxfont.Puts(x*60+5, y*60+15, str );
			color++;
		}
}

void UI_List_Mp3()
{
	//Блокировка энкодера, нужно чтобы обрабатывать в пре
	if (encoder_block == 0) {
		if (Encoder.Left) {
			Encoder.Left = 0;
			selectIndex-- ;

			if (selectIndex < 0) selectIndex = 0;

			if (selectIndex < window_start) {
				window_start = selectIndex;
				window_end = window_start + max_window_item;
			}

			//menu->field.needUpdate = 1;
			//tft.needUpdate = 1;
		}

		if (Encoder.Right) {
			Encoder.Right = 0;
			selectIndex++;

			if (selectIndex >= NUM)	selectIndex = NUM - 1;

			if (selectIndex > window_end) {
				window_end = selectIndex;
				window_start = window_end - max_window_item;
			}
			//tft.needUpdate = 1;
			//menu->field.needUpdate = 1;
		}
	}
	tft.RectangleFilled(7, 7 , 224, 150-3, 15); //Основной фон
	int ii = 0;
	for (int i = window_start; i <= window_end; i++) {
      if (i == selectIndex) tft.RectangleFilled(9, 10 + 20 * (ii % list_mp3.maxFileCount), 217, 20, 11);
      if ( i == selectIndex)
      {
    	tft.SetColor(15);
    	gfxfont.Puts(12, 30-3+20*(ii % (max_window_item + 1)), gfxfont.utf8rus2((char *)list_mp3.name[i].c_str()) , 16 );
      }
      else
      {
    	if (list_mp3.isDirectory[i])
    	  tft.SetColor(12);
        else
    	  tft.SetColor(4);

    	gfxfont.Puts(12, 30-3+20*(ii % (max_window_item+1)), gfxfont.utf8rus2((char *)list_mp3.name[i].c_str()) , 16 );
      }
	  ii++;
	}

    //┌───────────────────────────────────────────────────────────────────────┐
    //│ Рисуем вертикальный скролл //87 us                                    │
    //└───────────────────────────────────────────────────────────────────────┤
	int item_start_y = 10;
	int item_height = 20;
	//tft.RectangleFilled(218, item_start_y, 10,                    //│
	//		(max_window_item + 1) * item_height, 12); //│

		int H = ((max_window_item + 1) * item_height) - 4 + 4;
		float Hw = H * ((float) (max_window_item+1) / (float) (NUM));
		float delta = float(H - Hw)                                        //│
				/ (float) (NUM - max_window_item -1 );          //│

		tft.RectangleFilled(228,                                    //│
				item_start_y + delta * window_start,            //│
				2, Hw + 1, 16);                     //│
	                                                                        //│
	//────────────────────────────────────────────────────────────────────────┘
	if (KEY.isPress()) {

	  KEY.isHolded();
	  KEY.isDouble();

	  //Возврат назад
	  if ((selectIndex == 0) && (list_mp3.root == false)) //Реакция на 3 точки
	  {
		    //Пытаемся вернуться назад
		    //Для этого нужно выяснить прошлую директорию

		    globalPach = dirForvard(globalPach); //Получить прошлую директорию
		    rtt.println("globalPach %s", globalPach.c_str());

			memset( &list_mp3, 0 , sizeof(Dir_File_Info_Array));
			readDir((char *) globalPach.c_str(), &list_mp3);
			selectIndex = 0;
			window_start = 0;

		    if (globalPach == "/")
			  list_mp3.root = true;
		    else
		      addTreeDot(&list_mp3);

		    NUM = list_mp3.maxFileCount;
			window_end = (NUM >= max_window_item) ? max_window_item : NUM;
			return;
	  }

	  //Открыть директорию
	  if (list_mp3.isDirectory[selectIndex])
	  {
		    globalPach += "/";
			globalPach += list_mp3.name[selectIndex].c_str();
			memset( &list_mp3, 0 , sizeof(Dir_File_Info_Array));
			readDir((char *) globalPach.c_str(), &list_mp3);
			addTreeDot(&list_mp3);
			selectIndex = 0;
			window_start = 0;
		    NUM = list_mp3.maxFileCount;
			window_end = (NUM >= max_window_item) ? max_window_item : NUM;
			return;
	  }
	  else
	  {
		   //Запуск файла
		   play((char *)list_mp3.name[selectIndex].c_str());
	  }

	}






}


//Создание звднего фона
void Background_Board(void) {

	tft.Fill(0); //Окантовка

	tft.RectangleFilled(5, 5, 229, 229, 1); //Основной фон

	//tft.RectangleFilled(5, 5, 229, 229, 15); //Основной фон

	//Темное
	tft.LineH(5, 5, 234, 7);
	tft.LineH(6, 5, 234, 7);
	tft.LineV(5, 5, 234, 7);
	tft.LineV(6, 5, 234, 7);

	//Светлое
	tft.LineH(234, 5, 234, 9);
	tft.LineH(233, 6, 234, 9);
	tft.LineV(233, 6, 234, 9);
	tft.LineV(234, 5, 234, 9);

}

void PAGE_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str) {

	if (select) {
		tft.RectangleFilled(x, y, w, h, 2);
		tft.LineH(y, x, x + w, 3);
		tft.LineV(x, y, y + h, 3);

		tft.LineH(y + h, x + 1, x + w - 1, 4);
		tft.LineV(x + w, y + 1, y + h, 4);

		tft.LineH(y + h + 1, x + 1, x + w + 1, 15);
		tft.LineV(x + w + 1, y + 1, y + h + 1, 15);
	} else {
		tft.RectangleFilled(x, y, w, h, 1);
		tft.LineH(y, x, x + w, 5);
		tft.LineV(x, y, y + h, 5);

		tft.LineH(y + h, x + 1, x + w - 1, 6);
		tft.LineV(x + w, y + 1, y + h, 6);

		tft.LineH(y + h + 1, x + 1, x + w + 1, 15);
		tft.LineV(x + w + 1, y + 1, y + h + 1, 15);
	}

	gfxfont.set_delta_x(2);
	gfxfont.Puts(x + 15, y + 20, str);

	//u8g_SetFont(u8g_font_profont29);
	//uTFT_SetFontColor(BLACK, WHITE);
	//u8g_DrawStr(x + 15, y + (h+u8g.font_ref_ascent)/2, str );
}

//Нажатая кнопка
void PAGE_Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
		char *str) {

	tft.RectangleFilled(x, y, w, h, 2); // Светлый фон выбранной кнопки

	tft.LineH(y, x, x + w, 6);      // Темная часть окантовки темной кнопки
	tft.LineH(y + 1, x, x + w, 6);  // Темная часть окантовки темной кнопки

	tft.LineV(x, y, y + h, 6);      // Темная часть окантовки темной кнопки
	tft.LineV(x + 1, y, y + h, 6);  // Темная часть окантовки темной кнопки

	tft.LineH(y + h, x + 1, x + w - 1, 5); // Светлая часть окантовки темной кнопки
	tft.LineH(y + h - 1, x + 2, x + w - 1, 5); // Светлая часть окантовки темной кнопки

	tft.LineV(x + w, y + 1, y + h, 5);  // Светлая часть окантовки темной кнопки
	tft.LineV(x + w - 1, y + 2, y + h, 5); // Светлая часть окантовки темной кнопки

	tft.GotoXY(x + 15, 20);
	//gfxfont.set_delta_x(2);
	gfxfont.Puts(str);

	//u8g_SetFont(u8g_font_profont29);
	//uTFT_SetFontColor(BLACK, WHITE);
	//u8g_DrawStr(x + 15, y + (h+u8g.font_ref_ascent)/2, str );
}
