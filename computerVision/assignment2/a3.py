import numpy as np
import cv2 as cv

# 밝은 배경인 'img3_1.png' 이미지에서 잘 작동하고,
# 어두운 배경인 'img3_3.png', 'img3_4.png' 이미지에서 잘 작동합니다.
filename = 'img3_1.png'
src = cv.imread(filename, cv.IMREAD_COLOR)
cv.imshow(filename, src)

# 밝기를 기준으로 이진화를 적용하기 위해 YCrCb 색공간으로 변환합니다.
ycrcb = cv.cvtColor(src, cv.COLOR_BGR2YCrCb)
ycrcb_planes = cv.split(ycrcb)

# 평균 밝기가 어두울수록 배경이 어두운 경우이고, 평균 밝기가 밝을수록 배경이 밝은 경우이므로
# 평균 밝기가 100보다 작으면 밝은 부분을 객체로 인식하도록,
# 평균 밝기가 200보다 크면 어두운 부분을 객체로 인식하도록 합니다.
# 그리고 그 사이의 경우 히스토그램 평활화를 적용하여 명암비를 높인 후 이진화를 적용합니다.
if ycrcb_planes[0].mean() < 100:
    blur = cv.bilateralFilter(ycrcb_planes[0], -1, 3, 3)
    mask = cv.adaptiveThreshold(blur, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 101, -5)
elif ycrcb_planes[0].mean() > 200:
    blur = cv.bilateralFilter(ycrcb_planes[0], -1, 3, 3)
    mask = cv.adaptiveThreshold(blur, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY_INV, 101, -5)
else:
    blur = cv.equalizeHist(ycrcb_planes[0])
    blur = cv.bilateralFilter(blur, -1, 1, 1)
    mask = cv.adaptiveThreshold(blur, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 101, -5)

# 모폴로지 연산을 하여 mask의 잡음들을 제거합니다.
# 아래의 모폴로지 연산은 structure 변수와 iteration 파라미터를 조정해가며
# 실험하여 제 생각에 좋다고 판단된 값입니다.
structure = cv.getStructuringElement(cv.MORPH_CROSS, (3, 3))
mask = cv.morphologyEx(mask, cv.MORPH_ERODE, structure, iterations=7)
mask = cv.morphologyEx(mask, cv.MORPH_DILATE, structure, iterations=7)

cnt, _, stat, _ = cv.connectedComponentsWithStats(mask)

answer = []
for i in range(1, cnt):
    x, y, w, h, area = stat[i]
    tmp = blur[y:y+h, x:x+w]

    if w > h*1.5 or h > w*1.5 or w < 50:
        continue

    # 원본 이미지의 밝기 평면에서 해당 객체가 있는 부분에서 원을 검출합니다.
    # 각 파라미터 값들은 여러 실험을 통해 제 생각에 가장 좋다고 생각되는 값들입니다.
    circles = cv.HoughCircles(tmp, cv.HOUGH_GRADIENT, 1, 25, param1=150, param2=23, minRadius=10, maxRadius=w//3)

    if circles is None:
        continue

    # 가끔 잘못된 원이 검출되는데 이를 막고자 검출되는 원들의 반지름 평균보다 많이 큰 원들은 빼고 셉니다.
    radius = circles[0][:, 2]
    answer.append((radius <= (radius.mean()+5)).sum())

answer.sort()
print(answer)

cv.waitKey()
cv.destroyAllWindows()