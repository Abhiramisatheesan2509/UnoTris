#include <LedControl.h>
#include <avr/pgmspace.h>

// Pin configuration
const int DIN = 12;
const int CLK = 11;
const int CS = 10;
const int JS_X = A0;
const int JS_Y = A1;
const int JS_BTN = 2;

LedControl lc = LedControl(DIN, CLK, CS, 1);

// Game variables
unsigned long lastMoveTime = 0;
int gameDelay = 500; 
byte grid[8] = {0,0,0,0,0,0,0,0}; 

int pieceX, pieceY;
byte currentPiece[4][4];
int pieceType;

// Tetromino definitions
const byte shapes[7][4] = {
  {0x60, 0x60, 0x00, 0x00}, // Square
  {0x40, 0x40, 0x40, 0x40}, // Line
  {0x40, 0xE0, 0x00, 0x00}, // T
  {0x60, 0x30, 0x00, 0x00}, // S
  {0x30, 0x60, 0x00, 0x00}, // Z
  {0x40, 0x40, 0x60, 0x00}, // L
  {0x20, 0x20, 0x60, 0x00}  // J
};

// REDESIGNED "OVER" - Large, consistent letter sizes
const uint8_t OVER_TEXT[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, // O (Full size)
  0x3F, 0x40, 0x80, 0x40, 0x3F, 0x00, // V (Full size matching O)
  0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, // E
  0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, // R
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  
};

void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 5);
  lc.clearDisplay(0);
  pinMode(JS_BTN, INPUT_PULLUP);
  randomSeed(analogRead(A5)); 
  spawnPiece();
}

void gameOver() {
  for(int r=0; r<2; r++) {
    for(int i=0; i<8; i++) lc.setRow(0, i, 0xFF);
    delay(150);
    lc.clearDisplay(0);
    delay(150);
  }

  int textLength = sizeof(OVER_TEXT);
  
  // Left-to-Right Scrolling Logic
  for (int scroll = 0; scroll < textLength - 8; scroll++) {
    for (int col = 0; col < 8; col++) {
      lc.setColumn(0, 7 - col, pgm_read_byte(&OVER_TEXT[scroll + col]));
    }
    delay(100); 
  }

  delay(1000);
  for(int i=0; i<8; i++) grid[i] = 0;
  spawnPiece();
}

void spawnPiece() {
  pieceType = random(0, 7);
  pieceX = 2;
  pieceY = -1; 
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++) {
      currentPiece[i][j] = bitRead(shapes[pieceType][i], 7-j);
    }
  }
}

bool checkCollision(int nx, int ny) {
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++) {
      if(currentPiece[i][j]) {
        int gx = nx + j;
        int gy = ny + i;
        if(gx < 0 || gx >= 8 || gy >= 8) return true;
        if(gy >= 0 && bitRead(grid[gy], 7-gx)) return true;
      }
    }
  }
  return false;
}

void rotatePiece() {
  byte temp[4][4];
  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++)
      temp[j][3-i] = currentPiece[i][j];
      
  bool collision = false;
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++) {
      if(temp[i][j]) {
        int gx = pieceX + j;
        int gy = pieceY + i;
        if(gx < 0 || gx >= 8 || gy >= 8 || (gy >= 0 && bitRead(grid[gy], 7-gx))) collision = true;
      }
    }
  }

  if(!collision) {
    for(int i=0; i<4; i++)
      for(int j=0; j<4; j++)
        currentPiece[i][j] = temp[i][j];
  }
}

void settlePiece() {
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++) {
      if(currentPiece[i][j] && pieceY + i >= 0) {
        bitSet(grid[pieceY + i], 7 - (pieceX + j));
      }
    }
  }
  checkLines();
  spawnPiece();
  if(checkCollision(pieceX, pieceY)) {
    gameOver();
  }
}

void checkLines() {
  for(int i=0; i<8; i++) {
    if(grid[i] == 0xFF) { 
      for(int k=i; k>0; k--) grid[k] = grid[k-1];
      grid[0] = 0;
    }
  }
}

void draw() {
  lc.clearDisplay(0);
  for(int i=0; i<8; i++) {
    lc.setRow(0, 7 - i, grid[i]);
  }
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++) {
      if(currentPiece[i][j] && (pieceY + i) >= 0) {
        lc.setLed(0, 7 - (pieceY + i), pieceX + j, true);
      }
    }
  }
}

void loop() {
  int xIn = analogRead(JS_X);
  int yIn = analogRead(JS_Y);
  
  if(xIn < 300 && !checkCollision(pieceX - 1, pieceY)) {
    pieceX--;
    delay(150); 
  }
  if(xIn > 700 && !checkCollision(pieceX + 1, pieceY)) {
    pieceX++;
    delay(150);
  }
  
  if(digitalRead(JS_BTN) == LOW) {
    rotatePiece();
    delay(250); 
  }

  if(millis() - lastMoveTime > (yIn > 800 ? 50 : gameDelay)) {
    if(!checkCollision(pieceX, pieceY + 1)) {
      pieceY++;
    } else {
      settlePiece();
    }
    lastMoveTime = millis();
  }

  draw();
}
