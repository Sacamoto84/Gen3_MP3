/*
 * utiltites.cpp
 *
 *  Created on: 15 окт. 2022 г.
 *      Author: Ivan
 */
#include "global.h"
#include "fatfs.h"

/*ФУНКЦИЯ СОРТИРОВКИ СТРОК*/
void sort(mString <64> *S, int N, Dir_File_Info_Array * list)
{
	for (int i = 0; i < N - 1; i++)
			for (int j = i + 1; j < N; j++) {
				if (strcmp(S[i].c_str(), S[j].c_str())>0){
					//swap(S[i], S[j]);
					mString <64> temp = S[i];
					S[i] = S[j];
					S[j] = temp;

					bool tempb = list->isDirectory[i];
					list->isDirectory[i] = list->isDirectory[j];
					list->isDirectory[j] = tempb;
				}
	}

	for (int i = 0; i < N - 1; i++)
			for (int j = i + 1; j < N; j++) {

				if ((list->isDirectory[j] == true )&&(list->isDirectory[i] == false ))
				{
					mString <64> temp = list->name[i];
					list->name[i] = list->name[j];
					list->name[j] = temp;

					bool tempb = list->isDirectory[i];
					list->isDirectory[i] = list->isDirectory[j];
					list->isDirectory[j] = tempb;

					int tempi = list->size[i];
					list->size[i] = list->size[j];
					list->size[j] = tempi;
				}
	}

}

//Чтение дирректории и заполнение list_mp3
void readDir(char * path, Dir_File_Info_Array * list)
{
	mString <24> s;
	list->maxFileCount = 0;
	DIR dir;
	FILINFO fno;
	// открыть директорий 'music'
	FRESULT result = f_opendir(&dir, path);
	if (result != FR_OK)
	{
	  rtt.error("Невозможно открыть директорий; ошибка %d\n",(unsigned int)result);
	  stopError();
	}
	else
	{
	  rtt.successful("Директория %s открыта успешно", path);
	  rtt.info("..");

	  for (;;)
		  {
			result = f_readdir(&dir, &fno);                   /* Read a directory item */
		    if (result != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
		    //list_mp3.name[list_mp3.maxFileCount] = fno.fname;

		    if (fno.fattrib == 16)
              list->isDirectory[list->maxFileCount] = true;

		    char str[32];
		    sprintf(str,"%s", fno.fname);
		    tft.ConvertStringDosTo1251 ( str );
		    char strUTF8[48];
		    tft.ConvertString1251ToUTF8(str, strUTF8);
		    rtt.info(strUTF8);
		    list->name[list->maxFileCount] = strUTF8;

		    list->size[list->maxFileCount] = fno.fsize;

		    list->maxFileCount++;
		  }
		  f_closedir(&dir);
		}

		//Получили список файлов
		sort(list_mp3.name, list_mp3.maxFileCount, &list_mp3); //Сортируем массив
	    for(uint i = 0 ; i < list_mp3.maxFileCount; i++)
	    {
	       rtt.warning(list_mp3.name[i].c_str());
	    }

}

void stopError(void)
{
	rtt.setBold();
	rtt.setFlash();
	rtt.error("...stopError...");
	tft.setColorToPalete(15,tft.RGB565(255,255,0));
	tft.Fill(15);
	tft.Update();
	while(1){};
}


//Получить строку директории назад
mString <64> dirForvard(mString <64> dir)
{
  char temp[5][32];
  int x = 0 ;
  char * p = &dir.buf[1]; //Первый / игнорируем
  memset(&temp, 0, 5*32);
  int j = 0;
  while(*p)
  {
	if (*p == '/')
	{
		j++;
		x = 0;
	}
	temp[j][x++] = *p;
    p++;
  }

  mString <64> result;
  result.clear();
  if(j){
    for(int i = 0 ; i < j ; i++)
    {
	  result += "/";
	  result += (const char*)&temp[i][0];
    }
  }
  else
	result = "/";

  rtt.info("DirForvard dir %s", dir.c_str());
  rtt.info("result %s", result.c_str());
  return result;
}

//Добавить три точки
void addTreeDot(Dir_File_Info_Array * list){

	int orig_count = list->maxFileCount; //Исхоное количество файлов

	for(int i = orig_count ; i > 0 ; i--){
		list->name[i]        = list->name[i-1];
		list->isDirectory[i] = list->isDirectory[i-1];
		list->size[i]        = list->size[i-1];
	}

	list->maxFileCount++;

	for(uint i = 0; i < list->maxFileCount; i++)
	{
		if (list->isDirectory[i])
			list->size[i] = 555;
	}

	list->name[0] = "<-Back";
	list->isDirectory[0] = true;
	list->size[0] = 777;
}



