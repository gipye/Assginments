import numpy as np
import cv2 as cv

filename = 'img2_1.png'
src = cv.imread(filename, cv.IMREAD_GRAYSCALE)
cv.imshow(filename, src)

# 히스토그램 평활화를 해줍니다.
src = cv.equalizeHist(src)

# 블러링하여 잡음을 제거하고 이진화를 적용합니다.
blur = cv.bilateralFilter(src, -1, 1, 1)
_, mask = cv.threshold(blur, 128, 255, cv.THRESH_BINARY)

# 객체의 개수와 정보를 받아옵니다.
cnt, _, stat, _ = cv.connectedComponentsWithStats(mask)

answer = []     # 정답을 담을 변수
for i in range(1, cnt):
    x, y, w, h, area = stat[i]
    tmp = mask[y:y+h, x:x+w]    # 객체에 해당하는 부분만 tmp 변수가 가리키도록 합니다.

    # 객체가 조금이라도 회전되어 있을 경우 배경부분이 0인 부분이 있으므로 이를 255로 채워줍니다.
    for k in range(0, h):
        for j in range(0, w):
            if tmp[k, j] < 128:
                tmp[k, j] = 255
            else:
                break
        for j in range(w - 1, -1, -1):
            if tmp[k, j] < 128:
                tmp[k, j] = 255
            else:
                break
    for k in range(0, w):
        for j in range(0, h):
            if tmp[j, k] < 128:
                tmp[j, k] = 255
            else:
                break
        for j in range(h-1, -1, -1):
            if tmp[j, k] < 128:
                tmp[j, k] = 255
            else:
                break

    # 이제 tmp에는 배경이 흰색이고 눈금이 검은 색이므로 눈금만 추출하기 위해
    # 다시 이진화를 하여 눈금을 객체로 추출합니다.
    _, dst = cv.threshold(tmp, 128, 255, cv.THRESH_BINARY_INV)
    # 그리고 객체의 개수 즉 눈금의 개수를 answer 리스트에 추가합니다.
    tcnt, tlables = cv.connectedComponents(dst)
    answer.append(tcnt-1)

# 이제 answer 리스트를 정렬하여 콘솔에 출력합니다.
answer.sort()
print(answer)
cv.waitKey()
cv.destroyAllWindows()