#include "TextKeyboard.h"
#include "BoardConfig.h"
#include <string.h>

constexpr int TextKeyboard::MAX_TEXT;
constexpr int TextKeyboard::LETTER_ROWS;
constexpr int TextKeyboard::SPECIAL_ROW;
constexpr int TextKeyboard::SPECIAL_COUNT;

static const char *const KB_LOWER[4]={"1234567890","qwertyuiop","asdfghjkl","zxcvbnm./-"};
static const char *const KB_UPPER[4]={"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM./-"};
static const char *const KB_SYMBOL[4]={"1234567890",":/?#&=%@+","[]{}<>|~^","_-.,:;'!$"};
static const char *const KB_ACTIONS[5]={"SHIFT","SYM","SPACE","DEL","DONE"};

const char *TextKeyboard::rowChars(int index) const {
  if(index<0 || index>=LETTER_ROWS)return "";
  return symbols?KB_SYMBOL[index]:(upper?KB_UPPER[index]:KB_LOWER[index]);
}
int TextKeyboard::colsFor(int index) const {
  if(index==SPECIAL_ROW)return SPECIAL_COUNT;
  if(index==5)return masked?0:3;
  if(index>=0 && index<LETTER_ROWS)return (int)strlen(rowChars(index));
  return 0;
}
void TextKeyboard::open(const String &cap,const String &initial,bool password) {
  caption=cap;text=initial;
  if(text.length()>MAX_TEXT)text.remove(MAX_TEXT);
  masked=password;shown=true;done=cancel=false;
  upper=false;symbols=false;row=1;col=0;
  lastNumeric=Key::None;numericTap=0;numericAt=0;
}
bool TextKeyboard::insert(const char *s) {
  const size_t n=strlen(s);
  if(text.length()+n>MAX_TEXT)return false;
  text+=s;
  return true;
}
bool TextKeyboard::typeDigit(Key key) {
  static const char *const map[]={" 0",".,?1","abc2","def3","ghi4",
                                  "jkl5","mno6","pqrs7","tuv8","wxyz9"};
  int digit=int(key)-int(Key::Num0);
  if(digit<0||digit>9)return false;
  const char *seq=map[digit];
  const int count=(int)strlen(seq);
  uint32_t now=millis();
  if(lastNumeric==key && (uint32_t)(now-numericAt)<650 && text.length()) {
    numericTap=(numericTap+1)%count;
    char ch=seq[numericTap];
    if(upper&&ch>='a'&&ch<='z')ch-=32;
    text.remove(text.length()-1);
    text+=ch;
  }else if(text.length()<MAX_TEXT){
    numericTap=0;
    char ch=seq[0];
    if(upper&&ch>='a'&&ch<='z')ch-=32;
    text+=ch;
  }
  lastNumeric=key;numericAt=now;
  return true;
}
void TextKeyboard::moveRow(int delta) {
  const int old=col;
  const int total=masked?5:6;
  row=(row+total+delta)%total;
  const int n=colsFor(row);
  col=n?min(old,n-1):0;
}
void TextKeyboard::activate() {
  if(row<LETTER_ROWS) {
    char ch=rowChars(row)[col];
    char s[2]={ch,0};
    insert(s);
    return;
  }
  if(row==5 && !masked) {
    static const char *const tlds[]={".com",".net",".org"};
    if(col>=0&&col<3)insert(tlds[col]);
    return;
  }
  switch(col){
    case 0:upper=!upper;symbols=false;break;
    case 1:symbols=!symbols;break;
    case 2:insert(" ");break;
    case 3:if(text.length())text.remove(text.length()-1);break;
    case 4:done=true;shown=false;break;
  }
}
bool TextKeyboard::handle(const KeyEvent &e) {
  if(!shown || !e.pressed || e.longPress)return false;
  if(e.key>=Key::Num0 && e.key<=Key::Num9)return typeDigit(e.key);
  lastNumeric=Key::None;
  switch(e.key) {
    case Key::Left:col=(col+colsFor(row)-1)%colsFor(row);return true;
    case Key::Right:col=(col+1)%colsFor(row);return true;
    case Key::Up:moveRow(-1);return true;
    case Key::Down:moveRow(1);return true;
    case Key::Start:activate();return true;
    case Key::B:if(text.length())text.remove(text.length()-1);return true;
    case Key::Option:upper=!upper;symbols=false;return true;
    case Key::Select:symbols=!symbols;return true;
    case Key::Menu:done=true;shown=false;return true;
    case Key::A:cancel=true;shown=false;return true;
    default:return false;
  }
}
void TextKeyboard::draw(SymbianUI &ui,bool wifi,bool ble,bool sd,bool hour12) {
  auto &tft=ui.display();
  const auto colors=ui.c();
  ui.chrome(caption,wifi,ble,sd,hour12);
  // Repaint all keyboard pixels in-place. Never expose a clearContent()
  // frame during focus changes; keep OS status and softkey panes intact.
  tft.fillRect(0,29,Board::SCREEN_W,269,colors.bg);
  String visible;
  if(masked){for(size_t i=0;i<text.length();++i)visible+='*';}
  else visible=text;
  if(visible.length()>34)visible=visible.substring(visible.length()-34);
  tft.fillRect(5,37,230,29,colors.panel);
  tft.drawRect(5,37,230,29,colors.dim);
  tft.setTextSize(1);tft.setTextFont(1);
  tft.setTextColor(colors.text,colors.panel);
  tft.setCursor(10,47);tft.print(visible);
  for(int r=0;r<LETTER_ROWS;++r) {
    const char *chars=rowChars(r);
    const int n=colsFor(r);
    // 10 cells at 23 px pitch; rows 2/3 centered for QWERTY feel.
    const int start=5+(10-n)*11;
    const int y=81+r*28;
    for(int i=0;i<n;++i){
      const int x=start+i*23;
      const bool selected=row==r&&col==i;
      const uint16_t bg=selected?colors.selected:colors.panel;
      tft.fillRect(x,y,21,24,bg);
      tft.drawRect(x,y,21,24,selected?colors.border:colors.dim);
      char glyph[2]={chars[i],0};
      tft.setTextColor(colors.text,bg);
      tft.setCursor(x+7,y+8);tft.print(glyph);
    }
  }
  const int y=197;
  const int widths[5]={42,34,65,38,43};
  int x=5;
  for(int i=0;i<SPECIAL_COUNT;++i){
    const int w=widths[i];
    const bool selected=row==SPECIAL_ROW&&col==i;
    const uint16_t bg=selected?colors.selected:colors.panel;
    tft.fillRect(x,y,w,26,bg);
    tft.drawRect(x,y,w,26,selected?colors.border:colors.dim);
    tft.setTextColor(colors.text,bg);tft.setCursor(x+3,y+9);tft.print(KB_ACTIONS[i]);
    x+=w+1;
  }
  if(!masked) {
    const char *const shortcuts[]={".com",".net",".org"};
    for(int i=0;i<3;++i){
      const int tx=24+i*72;
      const bool selected=row==5&&col==i;
      const uint16_t bg=selected?colors.selected:colors.panel;
      tft.fillRect(tx,232,65,24,bg);
      tft.drawRect(tx,232,65,24,selected?colors.border:colors.dim);
      tft.setTextColor(colors.text,bg);tft.setCursor(tx+14,240);tft.print(shortcuts[i]);
    }
  }
  tft.setTextColor(colors.dim,colors.bg);
  tft.setCursor(6,270);
  tft.print("D-pad Move  OK Type  B Delete");
  ui.softkeys("Done",symbols?"ABC":(upper?"abc":"SYM"),"Cancel");
}
