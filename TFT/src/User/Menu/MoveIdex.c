#include "Move.h"
#include "includes.h"

//const GUI_RECT RecXYZ = {START_X + 1*ICON_WIDTH,        STATUS_GANTRY_YOFFSET,
//                         4*ICON_WIDTH+3*SPACE_X+START_X,ICON_START_Y-STATUS_GANTRY_YOFFSET};
#define X_MOVE_GCODE "G1 X%.2f F%d\n"
#define X_HOME_GCODE "G28 X\n"

#define CHANGE_TOOL0_GCODE "T0\n"
#define CHANGE_TOOL1_GCODE "T1\n"
const char *const changeToolCmd[] = {CHANGE_TOOL0_GCODE, CHANGE_TOOL1_GCODE};

#define GANTRY_UPDATE_DELAY 500  // 1 seconds is 1000

uint8_t item_move_len_index = 1;
uint8_t now_tool_index = 0;

typedef enum
{
  TOOL0 = 0,
  TOOL1
} TOOLS;

void storeMoveIdexCmd(TOOLS tool, int8_t direction)
{
  if(tool != now_tool_index){

    mustStoreCmd(changeToolCmd[tool]);
    // update now tool be selected
    now_tool_index = tool;
  }
  // if invert is true, 'direction' multiplied by -1
  storeCmd(X_MOVE_GCODE, (infoSettings.invert_axis[X_AXIS] ? -direction : direction) * moveLenSteps[item_move_len_index],
           infoSettings.xy_speed[infoSettings.move_speed]);
}

void storeHomeIdexCmd(TOOLS tool)
{
  if(tool != now_tool_index){

    mustStoreCmd(changeToolCmd[tool]);
    // update now tool be selected
    now_tool_index = tool;
  }
  storeCmd(X_HOME_GCODE);
}


#define LOAD_XYZ_LABEL_INDEX(p0, dir0, p1, dir1, axis) do{ \
                                                            moveIdexItems.items[p0].label.index = LABEL_##axis##_##dir0; \
                                                            moveIdexItems.items[p1].label.index = LABEL_##axis##_##dir1; \
                                                         }while(0)

void menuMoveIdex(void)
{
  MENUITEMS moveIdexItems = {
    // title
    LABEL_MOVE_IDEX,
    //   icon                          label
    {
        {ICON_X_DEC_IDEX_X1,           LABEL_X_DEC_IDEX_X1},
        {ICON_X_HOME_IDEX_X1,          LABEL_X_HOME_IDEX_X1},
        {ICON_X_INC_IDEX_X1,           LABEL_X_INC_IDEX_X1},
        {ICON_01_MM,                   LABEL_01_MM},
        {ICON_X_DEC_IDEX_X2,           LABEL_X_DEC_IDEX_X2},
        {ICON_X_HOME_IDEX_X2,          LABEL_X_HOME_IDEX_X2},
        {ICON_X_INC_IDEX_X2,           LABEL_X_INC_IDEX_X2},
        {ICON_BACK,                    LABEL_BACK},
    }
  };

  KEY_VALUES key_num = KEY_IDLE;

  mustStoreCmd("G91\n");
  mustStoreCmd("M114\n");

  // postion table of key
  uint8_t table[TOTAL_AXIS][2] =
    /*-------*-------*-------*---------*
     | X1-(0) | HomeX1+(1) | X1+(2) | unit(3) |
     *-------*-------*-------*---------*
     | X2-(4) | HomeX2-(5) | X2+(6) | back(7) |
     *-------*-------*-------*---------*/
    //X+ X-   Y+ Y-   Z+ Z-
    {{6, 4}, {1, 5}, {2, 0}};

  if (infoSettings.invert_axis[X_AXIS] != 1)
    LOAD_XYZ_LABEL_INDEX(table[X_AXIS][0], INC, table[X_AXIS][1], DEC, X);  // table[0] <--> INC(+) table[1] <--> DEC(+) if not inverted
  else
    LOAD_XYZ_LABEL_INDEX(table[X_AXIS][0], DEC, table[X_AXIS][1], INC, X);  // table[0] <--> DEC(-) table[1] <--> INC(-) if inverted

  moveIdexItems.items[KEY_ICON_3] = itemMoveLen[item_move_len_index];

  menuDrawPage(&moveIdexItems);
  drawXYZIdex();

  #if LCD_ENCODER_SUPPORT
    encoderPosition = 0;
  #endif

  while (infoMenu.menu[infoMenu.cur] == menuMoveIdex)
  {
    key_num = menuKeyGetValue();
    switch (key_num)
    {
        case KEY_ICON_0: storeMoveIdexCmd(TOOL0, -1); break;  // X1 move decrease if no invert
        case KEY_ICON_1: storeHomeIdexCmd(TOOL0); break;  // X1 home
        case KEY_ICON_2: storeMoveIdexCmd(TOOL0, 1); break;   // X1 move increase if no invert

        case KEY_ICON_3:
          item_move_len_index = (item_move_len_index + 1) % ITEM_MOVE_LEN_NUM;
          moveIdexItems.items[key_num] = itemMoveLen[item_move_len_index];
          menuDrawItem(&moveIdexItems.items[key_num], key_num);
          break;

        case KEY_ICON_4: storeMoveIdexCmd(TOOL1, -1); break;  // X2 move decrease if no invert
        case KEY_ICON_5: storeHomeIdexCmd(TOOL1); break;  // X2 home
        case KEY_ICON_6: storeMoveIdexCmd(TOOL1, 1); break;   // X2 move increase if no invert

        case KEY_ICON_7: infoMenu.cur--; break;

        default:
          #if LCD_ENCODER_SUPPORT
            if (encoderPosition)
            {
              storeMoveIdexCmd(now_tool_index, encoderPosition > 0 ? 1 : -1);
              encoderPosition = 0;
            }
          #endif
          break;
    }
    loopProcess();
    update_gantry_idex();
  }
  mustStoreCmd("G90\n");
}

void update_gantry_idex(void)
{
  if (nextScreenUpdate(GANTRY_UPDATE_DELAY))
  {
    coordinateQuery();
    drawXYZIdex();
  }
}

void drawXYZIdex(void)
{
  char tempstr[20];
  GUI_SetColor(INFOBOX_ICON_COLOR);

  sprintf(tempstr, "Tool:%d  ", now_tool_index);
  GUI_DispString(START_X, (ICON_START_Y - BYTE_HEIGHT) / 2, (uint8_t *)tempstr);

  sprintf(tempstr, "X:%.2f  ", coordinateGetAxisActual(X_AXIS));
  GUI_DispString(START_X + 1 * SPACE_X + 1 * ICON_WIDTH, (ICON_START_Y - BYTE_HEIGHT) / 2, (uint8_t *)tempstr);

  sprintf(tempstr, "Y:%.2f  ", coordinateGetAxisActual(Y_AXIS));
  GUI_DispString(START_X + 2 * SPACE_X + 2 * ICON_WIDTH, (ICON_START_Y - BYTE_HEIGHT) / 2, (uint8_t *)tempstr);

  sprintf(tempstr, "Z:%.2f  ", coordinateGetAxisActual(Z_AXIS));
  GUI_DispString(START_X + 3 * SPACE_X + 3 * ICON_WIDTH, (ICON_START_Y - BYTE_HEIGHT) / 2, (uint8_t *)tempstr);

  GUI_SetColor(infoSettings.font_color);
}
