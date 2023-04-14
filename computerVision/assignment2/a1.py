import numpy as np
import cv2 as cv

filename = 'img1_1.png'
src = cv.imread(filename, cv.IMREAD_GRAYSCALE)
cv.imshow(filename, src)

src = cv.equalizeHist(src)

blur = cv.bilateralFilter(src, -1, 1, 1)
_, mask = cv.threshold(blur, 128, 255, cv.THRESH_BINARY_INV)

cnt, _ = cv.connectedComponents(mask)

# cnt-1이 객체의 개수이고, 객체의 개수에는 배경도 포함되어있기 때문에
# cnt-2를 출력하여 주사위의 눈의 개수만 출력하게 합니다.
print(cnt-2)

cv.waitKey()
cv.destroyAllWindows()