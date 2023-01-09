extern void InitLCD(void) ; // initialises the LCD 2303eng should be printed
extern void DrawPixel(unsigned char x, unsigned char y) ; // draws a pixel at (x,y). Max size: 128 x 64 pixels
extern void ClearPixel(unsigned char x, unsigned char y) ; // clears a pixel at (x,y). Max size: 128 x 64 pixels
extern void ClearLCD(void) ; // clears entire LCD screen
extern void UpdateScreen(void) ; // Writes the screen buffer to the LCD
extern void PutcharLCD(char c) ; // Outputs the character in c to the LCD (ascii)
extern void Backlight(unsigned long colour) ; // Sets the backlight colour 1=G, 2=B, 4=R (you can mix to get other colours)
extern void CursorPos(unsigned char x, unsigned char y) ; // Sets the current cursor position. Max size :21 x 8 chars max

// Note the screen is 128 x 64 pixels
// Note the screen allows 21 x 8 characters

// All these functions have been optimised to increase the drawing speed over previous versions.
