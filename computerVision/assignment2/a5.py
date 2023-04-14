import numpy as np
import cv2 as cv

# 'img5_1.png', 'img5_2.png', 'img5_4.png', 'img5_5.png', 'img5_6.png',
# 'img5_7.png', 'img5_10.png', 'img5_11.png' 이미지에서 잘 작동합니다.
filename = 'img5_1.png'
src = cv.imread(filename, cv.IMREAD_COLOR)
cv.imshow(filename, src)

# 흑백 이미지로 변환 후 히스토그램 평활화를 하여 명암비를 높입니다.
src = cv.equalizeHist(cv.cvtColor(src, cv.COLOR_BGR2GRAY))

# 양방향 필터로 블러링을 하여 배경의 잡음만 효과적으로 제거합니다.
# 30번 정도 반복하니 배경의 잡음이 많이 제거됐습니다.
blur = cv.bilateralFilter(src, -1, 50, 10)
for i in range(30):
    blur = cv.bilateralFilter(blur, -1, 50, 1)

# 잡음이 제거된 이미지를 이진화하여 객체를 검출합니다.
# 이때 거의 주사위의 눈금만을 검출할 수 있습니다.
mask = cv.adaptiveThreshold(blur, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 11, -5)

# 원본 이미지에서 객체가 아닌 부분은 검은 색으로 채웁니다.
src[mask==0] = 0

# 이제 객체인 부분의 위치를 찾기 위해 connectedComponentsWithStats() 함수의 반환값을 얻습니다.
cnt, _, stat, _ = cv.connectedComponentsWithStats(mask)

# 원의 개수를 저장할 변수 count를 초기화합니다.
count = 0
for i in range(1, cnt):
    x, y, w, h, area = stat[i]
    tmp = src[y:y+h, x:x+w]

    # 가장 넓은 면에서의 눈금은 가장 원에 가까우므로 w와 h값이 비슷해야합니다.
    # 따라서 w값과 h값의 비가 많이 차이나면 무시합니다.
    # 또한 객체 크기가 매우 작으면 잡음으로 판단하여 무시하기 위해 w값이 10보다 큰 객체만 검사합니다.
    if w > h*1.5 or h > w*1.5 or w < 10:
        continue

    # 원본 이미지에서 원이 있다고 판단되는 부분에서 원을 찾습니다.
    circles = cv.HoughCircles(tmp, cv.HOUGH_GRADIENT, 1, 25, param1=200, param2=17, minRadius=10)

    # 가장 넓은 면이 아닌 면에서의 주사위 눈은 타원이어서 원이 검출되지 않습니다.
    # 원을 찾지 못했다면 잡음 객체이거나,
    # 가장 넓은 면에서의 눈이 아니어서  검출되지 않은 것입니다.
    # 따라서 원이 검출되지 않았다면 개수를 세지 않습니다.
    if circles is not None:
        if circles.shape[1] == 1:
            count += 1

# 이제 반복문이 끝나면 가장 넓은 면에서의 주사위 눈의 개수가 count 변수에 저장됩니다.
print(count)

cv.waitKey()
cv.destroyAllWindows()
