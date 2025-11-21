/* Scanner
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include các file cần thiết: reader.h, charcode.h, token.h, error.h
#include "reader.h"
#include "charcode.h"
#include "token.h"
#include "error.h"

// KHAI BÁO BIẾN TOÀN CỤC (Lấy từ các file khác sang)
// Mượn từ file read.c (Bộ đọc file)
extern int lineNo;      // dòng
extern int colNo;       // cột
extern int currentChar; // Ký tự đang đọc

extern CharCode charCodes[]; // Mượn từ file charcode.c (Bảng tra cứu thay mã ASCII)

/***************************************************************/

void skipBlank()
{
  while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_SPACE))
    readChar();
}

void skipComment()
{
  int state = 0;

  while (state != 2)
  {
    readChar(); // Đọc ký tự tiếp theo

    if (currentChar == EOF)
    {
      // Đang đọc mà hết file -> Lỗi
      error(ERR_ENDOFCOMMENT, lineNo, colNo);
    }

    switch (state)
    {
    case 0: // Trạng thái bình thường (Đang đọc nội dung)
      if (currentChar == '*')
      {
        state = 1; // Gặp dấu '*', chuyển sang trạng thái chờ dấu đóng
      }
      else
      {
        state = 0;
      }
      break;

    case 1: // Trạng thái vừa gặp dấu "*" (chờ dấu ')')
      if (currentChar == ')')
      {
        state = 2; // Gặp ')', -> '*)'
      }
      else if (currentChar == '*')
      {
        state = 1; // Vẫn là dấu '*', vẫn giữ trạng thái chờ (cho trường hợp '**')
      }
      else
      {
        state = 0; // quay lại trạng thái đầu
      }
      break;
    }
  }

  // Kết thúc vòng lặp nghĩa là đã gặp '*)'.
  readChar();
}

Token *readIdentKeyword(void)
{
  Token *token;
  int ln = lineNo, cn = colNo; // Lưu vị trí bắt đầu
  char buffer[MAX_IDENT_LEN + 1];
  int count = 0;

  // Vòng lặp Đọc Chữ cái HOẶC Chữ số
  while ((currentChar != EOF) &&
         ((charCodes[currentChar] == CHAR_LETTER) || (charCodes[currentChar] == CHAR_DIGIT)))
  {

    // Kiểm tra độ dài
    if (count < MAX_IDENT_LEN)
    {
      buffer[count] = (char)currentChar;
      count++;
    }
    else
    {
      error(ERR_IDENTTOOLONG, ln, cn);
    }
    readChar();
  }

  buffer[count] = '\0';

  // Kiểm tra xem từ vừa đọc có phải Từ khóa không?
  // Hàm checkKeyword bên file token.c
  TokenType type = checkKeyword(buffer);

  if (type != TK_NONE)
  {
    token = makeToken(type, ln, cn);
  }
  else
  {
    // else là tên biến (Identifier)
    token = makeToken(TK_IDENT, ln, cn);
  }

  strcpy(token->string, buffer);

  return token;
}

Token *readNumber(void)
{
  Token *token;
  int ln = lineNo, cn = colNo;
  char buffer[MAX_IDENT_LEN + 1];
  int count = 0;

  while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_DIGIT))
  {
    if (count < MAX_IDENT_LEN)
    {
      buffer[count] = (char)currentChar;
      count++;
    }
    readChar();
  }

  buffer[count] = '\0';

  token = makeToken(TK_NUMBER, ln, cn);

  strcpy(token->string, buffer);

  token->value = atoi(buffer);

  return token;
}

Token *readConstChar(void)
{
  Token *token;
  int ln = lineNo, cn = colNo; // Lưu vị trí dấu nháy mở đầu tiên

  readChar();

  // Kiểm tra lỗi
  if (currentChar == EOF)
  {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
  }

  // Kiểm tra lỗi '' (Rỗng)
  if (charCodes[currentChar] == CHAR_SINGLEQUOTE)
  {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
  }

  // Lưu ký tự
  char theChar = (char)currentChar;

  readChar();

  if (charCodes[currentChar] == CHAR_SINGLEQUOTE)
  {
    token = makeToken(TK_CHAR, ln, cn);

    token->string[0] = theChar;  // Lưu ký tự vào string
    token->string[1] = '\0';     // Khóa đuôi chuỗi
    token->value = (int)theChar; // Lưu mã ASCII vào value

    readChar();
    return token;
  }
  else
  {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
  }
  return NULL;
}

Token *getToken(void)
{
  Token *token;
  int ln, cn;

  if (currentChar == EOF)
    return makeToken(TK_EOF, lineNo, colNo);

  switch (charCodes[currentChar])
  {
  // KHOẢNG TRẮNG
  case CHAR_SPACE:
    skipBlank();
    return getToken(); // Gọi đệ quy để lấy token tiếp theo

  // CHỮ CÁI & SỐ
  case CHAR_LETTER:
    return readIdentKeyword();
  case CHAR_DIGIT:
    return readNumber();

  // DẤU
  case CHAR_PLUS: // Dấu +
    token = makeToken(SB_PLUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_MINUS: // Dấu -
    token = makeToken(SB_MINUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_TIMES: // Dấu *
    token = makeToken(SB_TIMES, lineNo, colNo);
    readChar();
    return token;
  case CHAR_SLASH: // Dấu /
    token = makeToken(SB_SLASH, lineNo, colNo);
    readChar();
    return token;
  case CHAR_EQ: // Dấu =
    token = makeToken(SB_EQ, lineNo, colNo);
    readChar();
    return token;
  case CHAR_COMMA: // Dấu ,
    token = makeToken(SB_COMMA, lineNo, colNo);
    readChar();
    return token;
  case CHAR_SEMICOLON: // Dấu ;
    token = makeToken(SB_SEMICOLON, lineNo, colNo);
    readChar();
    return token;
  case CHAR_PERIOD: // Dấu .
    token = makeToken(SB_PERIOD, lineNo, colNo);
    readChar();
    return token;
  case CHAR_SINGLEQUOTE: // Dấu ' (Hằng ký tự)
    return readConstChar();

  // DẤU PHỨC TẠP

  // Xử lý dấu ngoặc đơn '(' hoặc chú thích '(*'
  case CHAR_LPAR:
    ln = lineNo;
    cn = colNo;
    readChar(); // sang ký tự tiếp theo
    if (currentChar == '*')
    {
      // (* -> Bắt đầu chú thích
      skipComment();
      return getToken(); // Chú thích bị bỏ qua, lấy token tiếp
    }
    else if (currentChar == '.')
    {
      // Đây là (.
      token = makeToken(SB_LSEL, ln, cn);
      readChar();
      return token;
    }
    else
    {
      // là dấu ( bình thường
      return makeToken(SB_LPAR, ln, cn);
    }

  // Xử lý dấu Lớn hơn '>' hoặc '>='
  case CHAR_GT:
    ln = lineNo;
    cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ)
    {                                   // Nếu tiếp theo là =
      token = makeToken(SB_GE, ln, cn); // -> >=
      readChar();
    }
    else
    {
      token = makeToken(SB_GT, ln, cn); // -> >
    }
    return token;

  // Xử lý dấu Nhỏ hơn '<' hoặc '<=' hoặc '<>' (Khác)
  case CHAR_LT:
    ln = lineNo;
    cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ)
    {                                   // Nếu tiếp theo là =
      token = makeToken(SB_LE, ln, cn); // -> <=
      readChar();
    }
    else
    {
      token = makeToken(SB_LT, ln, cn); // -> <
    }
    return token;

  // Xử lý dấu Hai chấm ':' hoặc Phép gán ':='
  case CHAR_COLON:
    ln = lineNo;
    cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ)
    {                                       // Nếu tiếp theo là =
      token = makeToken(SB_ASSIGN, ln, cn); // -> := (Phép gán)
      readChar();
    }
    else
    {
      token = makeToken(SB_COLON, ln, cn); // -> :
    }
    return token;

  // Xử lý dấu đóng ngoặc ')'
  case CHAR_RPAR:
    token = makeToken(SB_RPAR, lineNo, colNo);
    readChar();
    return token;

  // LỖI
  default:
    token = makeToken(TK_NONE, lineNo, colNo);
    error(ERR_INVALIDSYMBOL, lineNo, colNo);
    readChar();
    return token;
  }
}

/******************************************************************/

void printToken(Token *token)
{

  printf("%d-%d:", token->lineNo, token->colNo);

  switch (token->tokenType)
  {
  case TK_NONE:
    printf("TK_NONE\n");
    break;
  case TK_IDENT:
    printf("TK_IDENT(%s)\n", token->string);
    break;
  case TK_NUMBER:
    printf("TK_NUMBER(%s)\n", token->string);
    break;
  case TK_CHAR:
    printf("TK_CHAR(\'%s\')\n", token->string);
    break;
  case TK_EOF:
    printf("TK_EOF\n");
    break;

  case KW_PROGRAM:
    printf("KW_PROGRAM\n");
    break;
  case KW_CONST:
    printf("KW_CONST\n");
    break;
  case KW_TYPE:
    printf("KW_TYPE\n");
    break;
  case KW_VAR:
    printf("KW_VAR\n");
    break;
  case KW_INTEGER:
    printf("KW_INTEGER\n");
    break;
  case KW_CHAR:
    printf("KW_CHAR\n");
    break;
  case KW_ARRAY:
    printf("KW_ARRAY\n");
    break;
  case KW_OF:
    printf("KW_OF\n");
    break;
  case KW_FUNCTION:
    printf("KW_FUNCTION\n");
    break;
  case KW_PROCEDURE:
    printf("KW_PROCEDURE\n");
    break;
  case KW_BEGIN:
    printf("KW_BEGIN\n");
    break;
  case KW_END:
    printf("KW_END\n");
    break;
  case KW_CALL:
    printf("KW_CALL\n");
    break;
  case KW_IF:
    printf("KW_IF\n");
    break;
  case KW_THEN:
    printf("KW_THEN\n");
    break;
  case KW_ELSE:
    printf("KW_ELSE\n");
    break;
  case KW_WHILE:
    printf("KW_WHILE\n");
    break;
  case KW_DO:
    printf("KW_DO\n");
    break;
  case KW_FOR:
    printf("KW_FOR\n");
    break;
  case KW_TO:
    printf("KW_TO\n");
    break;

  case SB_SEMICOLON:
    printf("SB_SEMICOLON\n");
    break;
  case SB_COLON:
    printf("SB_COLON\n");
    break;
  case SB_PERIOD:
    printf("SB_PERIOD\n");
    break;
  case SB_COMMA:
    printf("SB_COMMA\n");
    break;
  case SB_ASSIGN:
    printf("SB_ASSIGN\n");
    break;
  case SB_EQ:
    printf("SB_EQ\n");
    break;
  case SB_NEQ:
    printf("SB_NEQ\n");
    break;
  case SB_LT:
    printf("SB_LT\n");
    break;
  case SB_LE:
    printf("SB_LE\n");
    break;
  case SB_GT:
    printf("SB_GT\n");
    break;
  case SB_GE:
    printf("SB_GE\n");
    break;
  case SB_PLUS:
    printf("SB_PLUS\n");
    break;
  case SB_MINUS:
    printf("SB_MINUS\n");
    break;
  case SB_TIMES:
    printf("SB_TIMES\n");
    break;
  case SB_SLASH:
    printf("SB_SLASH\n");
    break;
  case SB_LPAR:
    printf("SB_LPAR\n");
    break;
  case SB_RPAR:
    printf("SB_RPAR\n");
    break;
  case SB_LSEL:
    printf("SB_LSEL\n");
    break;
  case SB_RSEL:
    printf("SB_RSEL\n");
    break;
  }
}

int scan(char *fileName)
{
  Token *token;

  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  token = getToken();
  while (token->tokenType != TK_EOF)
  {
    printToken(token);
    free(token);
    token = getToken();
  }

  free(token);
  closeInputStream();
  return IO_SUCCESS;
}

/******************************************************************/
