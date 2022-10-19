#include "global.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "playerTask.h"
#include "debugTask.h"
#include "main.h"
#include "global.h"

extern void DAC_DMA_Pause(void);
extern void DAC_DMA_Play(void);
extern void DAC_DMA_ClearBuffer(void);

void Background_Board(void);
void Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str);
void Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h, char *str);
void viwePalitra(void);
void UI_List_Mp3();

int selectIndex = 0;
int encoder_block = 0;

int max_window_item = 6;
int window_start    = 0;
int window_end;
int NUM;

bool mp3_config; //Режим управления перемоткой
bool pause;

void task_HMI(void)
{

  NUM = list_mp3.maxFileCount;
  window_end = (NUM >= max_window_item) ? max_window_item : NUM;

  while(1)
  {

	Background_Board();

	UI_List_Mp3();

	char str[64];

	sprintf(str, "%dk", playerInfo.filesize/1024);
	tft.SetColor(3);
	gfxfont.Puts(10, 228, str, 16);

	tft.SetColor(5);
	sprintf(str, "G%d", (int)(playerInfo.gain * 10.0F));
	gfxfont.Puts(188, 228, str, 16);

	tft.SetColor(3);
	sprintf(str, "E%d", playerInfo.error);
	gfxfont.Puts(105, 228, str, 16);

	tft.LineH(154, 7, 232, 6 );
	tft.LineH(155, 7, 232, 2 );

	tft.LineH(209, 7, 232, 6 );
	tft.LineH(210, 7, 232, 2 );

	//Индикатор
	tft.RectangleFilled(9, 187, 180 , 20, 15);
	tft.RectangleFilled(11, 189, (180-4)*playerInfo.fpersent , 16, 2);

	//Процент и пауза
	tft.RectangleFilled(191, 187, 40 , 20, 15);
	gfxfont.setFont(&DejaVu_Sans_Mono_12);
	if(pause == false){
	  tft.SetColor(3);
	  sprintf(str, "%4d",  (int)(playerInfo.fpersent*9999));
	  gfxfont.Puts(195, 201, str, 16);
	}
	else
	{
	   tft.SetColor(12);
	   gfxfont.Puts(192, 201, (char*)"Pause", 16 );
	}

	//Имя играенмого файла
	gfxfont.setFont(&CourierCyr12pt8b);
	tft.SetColor(14);
	sprintf(str, "%s",  playerInfo.filename.c_str());
	gfxfont.Puts(10, 178, gfxfont.utf8rus2(str), 16);

	//Перемотка
	if (mp3_config)
	{
		char * text = (char*)"< перемотка >";
		Button(20 ,100 , 200, 30 , 1, gfxfont.utf8rus2(text));
		int step = f_size(&SDFile)/4096/50;
		if (mp3_config) {

		  if (Encoder.Left) {
		    Encoder.Left = 0;
		    uint offset = f_tell(&SDFile) - (4096 * step);
		    f_lseek (&SDFile , offset );
		  }
		  if (Encoder.Right) {
		    Encoder.Right = 0;
		    uint offset = f_tell(&SDFile) + (4096 * step);
		    f_lseek (&SDFile , offset );
		  }
		}
	}
	tft.ST7789_UpdateDMA4bitV2();
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
	if (mp3_config == false) {
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
    	tft.SetColor(3);
    	gfxfont.Puts(12, 30-3+20*(ii % (max_window_item + 1)), gfxfont.utf8rus2((char *)list_mp3.name[i].c_str()) , 16 );
      }
      else
      {
    	if (list_mp3.isDirectory[i])
    	  tft.SetColor(12);
        else
    	  tft.SetColor(2);

    	gfxfont.Puts(12, 30-3+20*(ii % (max_window_item+1)), gfxfont.utf8rus2((char *)list_mp3.name[i].c_str()) , 16 );
      }
	  ii++;
	}

    //┌───────────────────────────────────────────────────────────────────────┐
    //│ Рисуем вертикальный скролл //87 us                                    │
    //└───────────────────────────────────────────────────────────────────────┤
	int item_start_y = 10;
	int item_height = 20;

	int H = ((max_window_item + 1) * item_height) - 4 + 4;

	if ((max_window_item+1) < NUM){
		float Hw = H * ((float) (max_window_item+1) / (float) (NUM));
		float delta = float(H - Hw) / (float) (NUM - max_window_item -1 );
		tft.RectangleFilled(228, item_start_y + delta * window_start, 2, Hw + 1, 16);
	}
	else
		tft.RectangleFilled(228, item_start_y, 2, H + 1, 16);

	//────────────────────────────────────────────────────────────────────────┘

	//KEY.isHolded();
	//KEY.isPress();
    //KEY.isRelease();

	KEY.tick();

	if (KEY.isSingle() && mp3_config)
	{
		mp3_config = false;
		return;
	}

	if (KEY.isHolded())
	{
	   mp3_config = !mp3_config;
	}
	else
	if (KEY.isSingle()) {

	  if (mp3_config)  return;

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
		   pause = false;
		   //Запуск файла
		   play((char *)list_mp3.name[selectIndex].c_str());

	  }

	}
	if (KEY.isDouble())
	{
		if (pause == false){
			DAC_DMA_Pause();
		    DAC_DMA_ClearBuffer(); //Очистить выходной буффер
			pause = true;
		}
		else
		{
		    DAC_DMA_ClearBuffer(); //Очистить выходной буффер
			DAC_DMA_Play();
			pause = false;
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

void Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str) {

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

	//gfxfont.set_delta_x(2);
	gfxfont.Puts(x + 10, y + 20, str);

	//u8g_SetFont(u8g_font_profont29);
	//uTFT_SetFontColor(BLACK, WHITE);
	//u8g_DrawStr(x + 15, y + (h+u8g.font_ref_ascent)/2, str );
}

//Нажатая кнопка
void Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
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
