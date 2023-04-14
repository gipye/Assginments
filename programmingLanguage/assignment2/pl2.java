import java.util.Scanner;

public class pl2 {

    static int charClass;
    static char[] lexeme = new char[100];
    static char nextChar;
    static int lexLen;
    static int nextToken;
    static char[] input = new char[4096];
    static int getChar_index;
    static int zero_divide;

    public static final int LETTER = 1;
    public static final int DIGIT = 2;
    public static final int UNKNOWN = 99;

    public static final int INT_LIT = 10;
    public static final int IDENT = 11;
    public static final int ADD_OP = 21;
    public static final int SUB_OP = 22;
    public static final int MULT_OP = 23;
    public static final int DIV_OP = 24;
    public static final int LEFT_PAREN = 25;
    public static final int RIGHT_PAREN = 26;
    public static final int ERR_OP = 30;

    public static void lookup(char ch) {
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
    }

    public static void addChar() {
        if(lexLen <= 98) {
            lexeme[lexLen++] = nextChar;
            lexeme[lexLen] = 0;
        }
        else
            System.out.printf("Error: lexeme is too long\n");
    }

    public static void getChar() {
        if( ( (nextChar = input[getChar_index]) ) != 0) {
            if(Character.isAlphabetic(nextChar))
                charClass = LETTER;
            else if(Character.isDigit(nextChar))
                charClass = DIGIT;
            else
                charClass = UNKNOWN;
        }

        else
            charClass = 0;

        getChar_index++;
    }

    public static void getNonBlank() {
        while(nextChar == ' ' || nextChar == '\t')
            getChar();
    }

    public static void lex() {
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

            case 0:
                lexeme[0] = 0;
                nextToken = 0;
                break;
        }	/* End of switch */

    }   /* End of function lex() */

    /* bnf의 expr을 수행하는 함수 */
    public static double expr() {
        double res;

        res = term();       //term은 무조건 있어야 하므로 바로 호출합니다.

        /* 다음 토큰이 + 거나 - 이면 +나 -가 더 이상 없을때까지 term을 호출하여 연산을 수행합니다. */
        while(nextToken == ADD_OP || nextToken == SUB_OP) {
            if(nextToken == ADD_OP) {
                lex();
                res += term();
            }

            else {
                lex();
                res -= term();
            }
        }

        return res;
    }

    /* bnf의 term을 수행하는 함수 */
    public static double term() {
        double res;

        res = factor();     //factor은 무조건 있어야 하므로 바로 호출합니다.

        /* 다음 토큰이 * 거나 / 이면 *나 /가 더 이상 없을때까지 factor을 호출하여 연산을 수행합니다. */
        while(nextToken == MULT_OP || nextToken == DIV_OP) {
            if(nextToken == MULT_OP) {
                lex();
                res *= factor();
            }

            else {
                lex();
                double divisor;
                divisor = factor();
                if(divisor == 0)
                    zero_divide = 1;
                else
                    res /= divisor;
            }
        }

        return res;
    }

    /* bnf의 factor을 수행하는 함수 */
    public static double factor() {
        double res = 0;

        //토큰이 -로 시작할 경우입니다.
        if(nextToken == SUB_OP) {
            lex();      //-를 없애줍니다.
            if(nextToken == INT_LIT) {
                res = 0 - number();     //음수여야 하므로 number()의 리턴값을 음수로 res에 넣어줍니다.
            }

            else if(nextToken == LEFT_PAREN) {
                lex();
                res = 0 - expr();       //음수여야 하므로 expr()의 계산값을 음수로 넣어줍니다.
                if(nextToken != RIGHT_PAREN) {      //닫는 괄호가 없으면 에러입니다.
                    System.out.printf("Syntax error!!\n");
                    System.exit(1);
                }
            }

            else {
                System.out.printf("Syntax error\n");
                System.exit(1);
            }
        }

        //토큰이 -로 시작하지 않을 경우입니다.
        else {
            if(nextToken == INT_LIT) {
                res = number();     //number()의 리턴값을 넣어줍니다.
            }

            else if(nextToken == LEFT_PAREN) {
                lex();      //(을 없애줍니다.
                res = expr();   //expr()의 계산 값을 넣어줍니다.
                if(nextToken != RIGHT_PAREN) {
                    System.out.printf("Syntax error!!\n");
                    System.exit(1);
                }
            }

            else {
                System.out.printf("Syntax error!!\n");
                System.exit(1);
            }
        }
        lex();
        return res;
    }

    /* number을 수행하는 함수입니다. digit는 안 만들어도 된다고 하셔서 여기서 숫자로 변환합니다. */
    public static double number() {
        int i;
        double res = (float)(lexeme[0] - '0');  //가장 큰 자리수의 숫자를 넣어줍니다.

        for(i = 0; i < 99; i++) {       //토큰의 자리수를 얻어냅니다.
            if(lexeme[i] == 0)
                break;
        }

        for(int j = 1; j < i; j++) {        //토큰의 자리수만큼 반복하며 실수형으로 저장합니다.
            res *= 10;
            res += (float)(lexeme[j] - '0');
        }

        return res;
    }

    /*정수인지 실수인지 확인하는 함수입니다. */
    public static int is_digit(double f){
        int i = (int)f;         //f를 형변환하여 소수부분을 없애고 i에 대입합니다.
        if((double)i == f)      //만약 i와 f가 같다면 f는 처음부터 정수라는 의미이므로 0을 반환합니다.
            return 0;
        else
            return 1;
    }

    public static void main(String[] argv) {
        double res;
        Scanner scanner = new Scanner(System.in);
        String input_buf;

        while(true) {
            /* 변수 초기화 */
            for(int i = 0; i < 100; i++)
                input[i] = 0;
            getChar_index = 0;
            zero_divide = 0;

            System.out.printf(">> ");
            input_buf = scanner.nextLine();     //문자열을 입력받아 char형 배열에 넣어줍니다.
            for(int i = 0; i < input_buf.length(); i++)
                input[i] = input_buf.charAt(i);
            input[input_buf.length()] = 0;      //마지막 원소에는 0을 넣습니다.

            if(input[0] == 0)
                continue;

            /* 토큰을 얻어 expr을 수행합니다. */
            getChar();
            lex();
            res = expr();

            /* expr을 수행했는데 토큰이 남아있으면 에러입니다. */
            if(nextToken != 0) {
                System.out.printf("Syntax error!!\n");
                System.exit(1);
            }

            /* 정수면 정수로 실수면 실수로 출력합니다. */
            else {
                if (zero_divide == 1)
                    System.out.printf("Error: Cannot be divided by zero.\n");
                else if (is_digit(res) == 1)
                    System.out.printf("%f\n", res);
                else
                    System.out.printf("%d\n", (int)res);
            }
        }
    }
}
