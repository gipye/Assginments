#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 10000

// 행렬곱 함수
int **multiplyMatrix(int ***, int ***, int);

// 행렬을 담을 배열 메모리 관리 함수
int **allocMatrix(int);
void freeMatrix(int **, int);
// 슈트라센 알고리즘에 적용하기 위해 행렬 크기를 2의 제곱수로 맞춰주는 함수
int initMatrix(int ***, int);
// 행렬 크기를 원래대로 되돌리는 함수
void rollbackMatrix(int ***, int);
// 슈트라센 알고리즘 에서 사용할 덧셈, 뺄셈 재귀 함수
int **addMatrix(int **, int **, int);
int **subMatrix(int **, int **, int);
int **strassen(int **, int **, int);

// 행렬을 출력하는 함수
void printMatrix(int **matrix, int);


// main 함수
int main() {
    int increase_number = 1;

    int **a = allocMatrix(N);
    int **b = allocMatrix(N);

    // 행렬 초기화
    // 이 부분에 행렬을 초기화하는 기능을 구현합니다.
    // 사용자로부터 입력을 받거나 랜덤으로 값을 생성하는 등의 코드를 작성합니다.
    // 저는 정답이 올바른지 확인하기 쉽게 하기 위해 일정하게 증가하는 값을 넣었습니다.
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            a[i][j] = increase_number;
            b[i][j] = increase_number++;
        }
    }

    // 행렬 곱셈 수행
    int **c = multiplyMatrix(&a, &b, N);

    // 행렬 a, b, c 출력
    printf("a:\n");
    printMatrix(a, N);
    printf("b:\n");
    printMatrix(b, N);
    printf("axb:\n");
    printMatrix(c, N);

    // 할당된 메모리 해제
    freeMatrix(a, N);
    freeMatrix(b, N);
    freeMatrix(c, N);

    return 0;
}


// 여기부터 함수 구현부

int **allocMatrix(int n) {
    int **matrix = (int **)malloc(sizeof(int*)*n);
    for(int i = 0; i < n; i++)
        matrix[i] = (int *)malloc(sizeof(int)*n);
    return matrix;
}

void freeMatrix(int **matrix, int n) {
    for(int i = 0; i < n; i++)
        free(matrix[i]);
    free(matrix);
}

int initMatrix(int ***matrix, int n) {
    int **newMatrix;
    int newSize = 1;

    while(n > newSize)
        newSize *= 2;

    newMatrix = allocMatrix(newSize);
    for(int i = 0; i < newSize; i++)
        memset(newMatrix[i], 0, newSize);

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            newMatrix[i][j] = (*matrix)[i][j];
        }
    }
    freeMatrix(*matrix, n);

    *matrix = newMatrix;

    return newSize;
}

void rollbackMatrix(int ***matrix, int n) {
    int **newMatrix = allocMatrix(sizeof(int *)*n);

    for(int i = 0; i < n; i++)
        memcpy(newMatrix[i], (*matrix)[i], sizeof(int)*n);
    
    freeMatrix(*matrix, n);

    *matrix = newMatrix;
}

int **addMatrix(int **a, int **b, int n) {
    int **c = allocMatrix(n);

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c[i][j] = a[i][j] + b[i][j];
        }
    }
    return c;
}

int **subMatrix(int **a, int **b, int n) {
    int **c = allocMatrix(n);

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c[i][j] = a[i][j] - b[i][j];
        }
    }
    return c;
}

int **strassen(int **a, int **b, int n) {
    if (n == 1) {
        int **c = allocMatrix(1);
        c[0][0] = a[0][0] * b[0][0];
        return c;
    }

    int newSize = n / 2;

    int **a11 = allocMatrix(newSize);
    int **a12 = allocMatrix(newSize);
    int **a21 = allocMatrix(newSize);
    int **a22 = allocMatrix(newSize);
    int **b11 = allocMatrix(newSize);
    int **b12 = allocMatrix(newSize);
    int **b21 = allocMatrix(newSize);
    int **b22 = allocMatrix(newSize);

    // a 행렬을 4개의 부분 행렬로 분할
    for (int i = 0; i < newSize; i++) {
        for (int j = 0; j < newSize; j++) {
            a11[i][j] = a[i][j];
            a12[i][j] = a[i][j + newSize];
            a21[i][j] = a[i + newSize][j];
            a22[i][j] = a[i + newSize][j + newSize];

            b11[i][j] = b[i][j];
            b12[i][j] = b[i][j + newSize];
            b21[i][j] = b[i + newSize][j];
            b22[i][j] = b[i + newSize][j + newSize];
        }
    }

    int **m1, **m2, **m3, **m4, **m5, **m6, **m7;
    int **tmp1, **tmp2;

    // m1 계산
    tmp1 = addMatrix(a11, a22, newSize);
    tmp2 = addMatrix(b11, b22, newSize);
    m1 = strassen(tmp1, tmp2, newSize);
    freeMatrix(tmp1, newSize);
    freeMatrix(tmp2, newSize);

    // m2 계산
    tmp1 = addMatrix(a21, a22, newSize);
    m2 = strassen(tmp1, b11, newSize);
    freeMatrix(tmp1, newSize);

    // m3 계산
    tmp1 = subMatrix(b12, b22, newSize);
    m3 = strassen(a11, tmp1, newSize);
    freeMatrix(tmp1, newSize);

    // m4 계산
    tmp1 = subMatrix(b21, b11, newSize);
    m4 = strassen(a22, tmp1, newSize);
    freeMatrix(tmp1, newSize);

    // m5 계산
    tmp1 = addMatrix(a11, a12, newSize);
    m5 = strassen(tmp1, b22, newSize);
    freeMatrix(tmp1, newSize);

    // m6 계산
    tmp1 = subMatrix(a21, a11, newSize);
    tmp2 = addMatrix(b11, b12, newSize);
    m6 = strassen(tmp1, tmp2, newSize);
    freeMatrix(tmp1, newSize);
    freeMatrix(tmp2, newSize);

    // m7 계산
    tmp1 = subMatrix(a12, a22, newSize);
    tmp2 = addMatrix(b21, b22, newSize);
    m7 = strassen(tmp1, tmp2, newSize);
    freeMatrix(tmp1, newSize);
    freeMatrix(tmp2, newSize);

    int **c11, **c12, **c21, **c22;

    tmp1 = addMatrix(m1, m4, newSize);
    tmp2 = subMatrix(tmp1, m5, newSize);
    c11 = addMatrix(tmp2, m7, newSize);
    freeMatrix(tmp1, newSize);
    freeMatrix(tmp2, newSize);

    c12 = addMatrix(m3, m5, newSize);
    c21 = addMatrix(m2, m4, newSize);

    tmp1 = subMatrix(m1, m2, newSize);
    tmp2 = addMatrix(tmp1, m3, newSize);
    c22 = addMatrix(tmp2, m6, newSize);
    freeMatrix(tmp1, newSize);
    freeMatrix(tmp2, newSize);

    int **c = allocMatrix(n);

    // 결과 행렬 c에 부분 행렬들을 결합
    for (int i = 0; i < newSize; i++) {
        for (int j = 0; j < newSize; j++) {
            c[i][j] = c11[i][j];
            c[i][j + newSize] = c12[i][j];
            c[i + newSize][j] = c21[i][j];
            c[i + newSize][j + newSize] = c22[i][j];
        }
    }

    // 할당된 메모리 해제
    freeMatrix(m1, newSize);
    freeMatrix(m2, newSize);
    freeMatrix(m3, newSize);
    freeMatrix(m4, newSize);
    freeMatrix(m5, newSize);
    freeMatrix(m6, newSize);
    freeMatrix(m7, newSize);
    freeMatrix(c11, newSize);
    freeMatrix(c12, newSize);
    freeMatrix(c21, newSize);
    freeMatrix(c22, newSize);
    freeMatrix(a11, newSize);
    freeMatrix(a12, newSize);
    freeMatrix(a21, newSize);
    freeMatrix(a22, newSize);
    freeMatrix(b11, newSize);
    freeMatrix(b12, newSize);
    freeMatrix(b21, newSize);
    freeMatrix(b22, newSize);

    return c;
}

int **multiplyMatrix(int ***a, int ***b, int n) {
    int newSize = initMatrix(a, n);
    initMatrix(b, n);

    int **c = strassen(*a, *b, newSize);

    rollbackMatrix(a, N);
    rollbackMatrix(b, N);
    rollbackMatrix(&c, N);

    return c;
}

void printMatrix(int **matrix, int n) {
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            printf("%5d |", matrix[i][j]);
        }
        printf("\n");
    }
}
