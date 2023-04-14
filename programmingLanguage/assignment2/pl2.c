#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>

/*전역변수*/
int charClass;
char lexeme[100];
char nextChar;
int lexLen;
int token;
int nextToken;
char input[4096];
int getChar_index;
int zero_divide;

/* lexical analyzer function */
int lookup(char);
void addChar();
void getChar();
void getNonBlank();
int lex();

/* RD parser function */
double expr();
double term();
double factor();
double number();

/* decimal number인지 판별 */
int is_decimal(double);

/* Character classes */
#define LETTER 1
#define DIGIT 2
#define UNKNOWN 99

/* Token codes */
#define INT_LIT 10
#define IDENT 11
#define ASSIGN_OP 20
#define ADD_OP 21
#define SUB_OP 22
#define MULT_OP 23
#define DIV_OP 24
#define LEFT_PAREN 25
#define RIGHT_PAREN 26
#define ERR_OP 30

int main() {
	char c;
	double res;

	while(1) {
		/* 변수 초기화 */
		memset(input, 0, sizeof(input));
		getChar_index = 0;
		zero_divide = 0;

		printf(">> ");
		scanf("%[ \t]", input);
		scanf("%[^\n]", input);		//문자열을 입력받아 char형 배열에 넣어줍니다.
		c = getchar();
		if(input[0] == 0)
			continue;

		/* 토큰을 얻어 expr을 수행합니다. */
		getChar();
		lex();
		res = expr();

		/* expr을 수행했는데 토큰이 남아있으면 에러입니다. */
		if(nextToken != 0) {
			printf("Syntax error!!\n");
			exit(1);
		}

		/* 정수면 정수로 실수면 실수로 출력합니다. */
		else {
			if(zero_divide == 1)
				printf("Error: Cannot be divided by zero.\n");
			else if(is_decimal(res))
				printf("%f\n", res);
			else
				printf("%d\n", (int)res);
		}
	}

	exit(0);
}

int lookup(char ch) {
	switch(ch) {
		case '(':
			addChar();
			nextToken = LEFT_PAREN;
			break;

		case ')':
			addChar();
			nextToken = RIGHT_PAREN;
			break;

		case '+':
			addChar();
			nextToken = ADD_OP;
			break;

		case '-':
			addChar();
			nextToken = SUB_OP;
			break;

		case '*':
			addChar();
			nextToken = MULT_OP;
			break;

		case '/':
			addChar();
			nextToken = DIV_OP;
			break;

		default:
			addChar();
			nextToken = ERR_OP;
			break;
	}
	return nextToken;
}

void addChar() {
	if(lexLen <= 98) {
		lexeme[lexLen++] = nextChar;
		lexeme[lexLen] = 0;
	}
	else
		printf("Error: lexeme is too long \n");
}

void getChar() {
	if( ( (nextChar = input[getChar_index]) ) != '\0') {
		if(isalpha(nextChar))
			charClass = LETTER;
		else if(isdigit(nextChar))
			charClass = DIGIT;
		else
			charClass = UNKNOWN;
	}

	else
		charClass = '\0';

	getChar_index++;
}

void getNonBlank() {
	while(isspace(nextChar))
		getChar();
}

int lex() {
	lexLen = 0;
	getNonBlank();
	switch (charClass) {
		case LETTER:
			addChar();
			getChar();
			while(charClass == LETTER || charClass == DIGIT) {
				addChar();
				getChar();
			}
			nextToken = IDENT;
			break;

		case DIGIT:
			addChar();
			getChar();
			while(charClass == DIGIT) {
				addChar();
				getChar();
			}
			nextToken = INT_LIT;
			break;

		case UNKNOWN:
			lookup(nextChar);
			getChar();
			break;

		case '\0':
			lexeme[0] = 0;
			nextToken = 0;
			break;
		}	/* End of switch */

	return nextToken;
}	/* End of function lex() */

/* bnf의 expr을 수행하는 함수 */
double expr() {
	double res = 0;

	res = term();	//term은 무조건 있어야 하므로 바로 호출합니다.

	/* 다음 토큰이 + 거나 - 이면 +나 -가 더 이상 없을때까지 term을 호출하여 연산을 수행합니다. */
	while(nextToken == ADD_OP || nextToken == SUB_OP) {
		if(nextToken == ADD_OP) {
			lex();		//+를 없애줍니다.
			res += term();	//term의 계산 값을 더해서 저장합니다.
		}

		else {
			lex();		//-를 없애줍니다.
			res -= term();	//term의 계산 값을 빼서 저장합니다.
		}
	}

	return res;
}

/* bnf의 term을 수행하는 함수 */
double term() {
	double res = 0;

	res = factor();		//factor은 무조건 있어야 하므로 바로 호출합니다.

	/* 다음 토큰이 * 거나 / 이면 *나 /가 더 이상 없을때까지 factor을 호출하여 연산을 수행합니다. */
	while(nextToken == MULT_OP || nextToken == DIV_OP) {
		if(nextToken == MULT_OP) {
			lex();		//*를 없애줍니다.
			res *= factor();	//factor의 계산값을 곱해서 저장합니다.
		}

		else {
			lex();		///를 없애줍니다.
			double divisor;
			divisor = factor();

			/* 0으로 나눌 시 에러처리합니다. */
			if(divisor == 0)
				zero_divide = 1;
			else
				res /= divisor;	//factor의 계산값으로 나눠서 저장합니다.
		}
	}

	return res;
}

/* bnf의 factor을 수행하는 함수 */
double factor() {
	double res = 0;

	//토큰이 -로 시작할 경우입니다.
	if(nextToken == SUB_OP) {
		lex();		//-를 없애줍니다.
		if(nextToken == INT_LIT) {
			res = 0 - number();		//음수여야 하므로 number()의 리턴값을 음수로 res에 넣어줍니다.
		}

		else if(nextToken == LEFT_PAREN) {
			lex();
			res = 0 - expr();		//음수여야 하므로 expr()의 계산값을 음수로 넣어줍니다.
			if(nextToken != RIGHT_PAREN) {
				printf("Syntax error!!\n");
				exit(1);
			}
		}

		else {
			printf("Syntax error\n");
			exit(1);
		}
	}

	else {
		if(nextToken == INT_LIT) {
			res = number();		//number()의 리턴값을 넣어줍니다.
		}

		else if(nextToken == LEFT_PAREN) {
			lex();		//(을 없애줍니다.
			res = expr();	//expr()의 계산 값을 넣어줍니다.
			if(nextToken != RIGHT_PAREN) {
				printf("Syntax error!!\n");
				exit(1);
			}
		}

		else {
			printf("Syntax error!!\n");
			exit(1);
		}
	}
	lex();
	return res;
}

/* number을 수행하는 함수입니다. digit는 안 만들어도 된다고 하셔서 여기서 숫자로 변환합니다. */
double number() {
	double res = 0;

	res = (double)(lexeme[0] - '0');	//가장 큰 자리수의 숫자를 넣어줍니다.

	/* 토큰의 자리수만큼 반복하며 실수형으로 저장합니다. */
	for(int i = 1; i < strlen(lexeme); i++) {
		res *= 10;
		res += lexeme[i] - '0';
	}

	return res;
}

/*정수인지 실수인지 확인하는 함수입니다. */
int is_decimal(double f) {
	int i;
	i = f;	//f를 형변환하여 소수부분을 없애고 i에 대입합니다.

	if((double)i == f)
		return 0;	//만약 i와 f가 같다면 f는 처음부터 정수라는 의미이므로 0을 반환합니다.
	else
		return 1;
}
