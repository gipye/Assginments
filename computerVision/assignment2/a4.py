import numpy as np
import cv2 as cv

# 이진화 된 이미지에서 두 객체가 붙어있는 경우 한 객체로 인식되기 때문에
# 이를 나누어주는 함수를 구현했습니다.
# mask는 이진화 된 이미지, iteration은 반복 횟수
def cut(mask, iteration):
    for i in range(iteration):
        mask_expend = np.zeros((mask.shape[0]+4, mask.shape[1]+4), 'uint8')
        count = np.zeros(mask.shape, 'uint8')
        mask_expend[2:mask_expend.shape[0]-2, 2:mask_expend.shape[1]-2] = mask

        # 각 변수는 mask 이미지를 기준으로 왼쪽 위, 위, 오른쪽 위, 왼쪽, 오른쪽, 왼쪽 아래, 아래, 오른쪽 아래의 값을 가지는 이미지입니다.
        mask1 = mask_expend[1:mask_expend.shape[0]-3, 1:mask_expend.shape[1]-3]
        mask2 = mask_expend[1:mask_expend.shape[0]-3, 2:mask_expend.shape[1]-2]
        mask3 = mask_expend[1:mask_expend.shape[0]-3, 3:mask_expend.shape[1]-1]
        mask4 = mask_expend[2:mask_expend.shape[0]-2, 1:mask_expend.shape[1]-3]
        mask6 = mask_expend[2:mask_expend.shape[0]-2, 3:mask_expend.shape[1]-1]
        mask7 = mask_expend[3:mask_expend.shape[0]-1, 1:mask_expend.shape[1]-3]
        mask8 = mask_expend[3:mask_expend.shape[0]-1, 2:mask_expend.shape[1]-2]
        mask9 = mask_expend[3:mask_expend.shape[0]-1, 3:mask_expend.shape[1]-1]

        # 어떤 픽셀이 8방향 중 6방향이 객체가 있다면 한 객체로 판단하고,
        # 아니라면 두 객체의 접점으로 판단하도록 하려고 합니다.
        # 그래서 count 변수에 몇 방향으로 객체가 있는지 값을 담아둡니다.
        count[mask1 > 0] = count[mask1 > 0] + mask[mask1 > 0]/255
        count[mask2 > 0] = count[mask2 > 0] + mask[mask2 > 0]/255
        count[mask3 > 0] = count[mask3 > 0] + mask[mask3 > 0]/255
        count[mask4 > 0] = count[mask4 > 0] + mask[mask4 > 0]/255
        count[mask6 > 0] = count[mask6 > 0] + mask[mask6 > 0]/255
        count[mask7 > 0] = count[mask7 > 0] + mask[mask7 > 0]/255
        count[mask8 > 0] = count[mask8 > 0] + mask[mask8 > 0]/255
        count[mask9 > 0] = count[mask9 > 0] + mask[mask9 > 0]/255

        # 이제 6방향 이상 객체가 있는 것이 아니라면 두 객체를 나누기 위해
        # 마스크 이미지에서 해당되는 픽셀의 값을 0으로 만듭니다.
        mask[count < 6] = 0

    # 이후 객체가 깍일 수 밖에 없으므로 모폴로지 연산 중 팽창 연산을 수행합니다.
    # 많이 하면 두 객체가 다시 붙을 수 있으므로 파라미터로 온 반복 횟수의 반만큼 수행하도록 합니다.
    structure = cv.getStructuringElement(cv.MORPH_CROSS, (3, 3))
    mask = cv.morphologyEx(mask, cv.MORPH_DILATE, structure, iterations=iteration//2)
    return mask

#img4_2.png 파일에 대해 정상적으로 작동
filename = 'img4_2.png'
src = cv.imread(filename, cv.IMREAD_COLOR)
cv.imshow(filename, src)

# 컬러 이미지에서 객체를 쉽게 구분할 수 있도록
# 컬러 정보만 한 채널에 있는 hsv 색공간으로 변환하여 채널을 분할합니다.
hsv = cv.cvtColor(src, cv.COLOR_BGR2YCrCb)
hsv_planes = cv.split(hsv)

# 컬러 정보가 담긴 채널을 히스토그램 평활화하여 객체를 잘 구분할 수 있도록 합니다.
equal = cv.equalizeHist(hsv_planes[0])

# 그리고 컬러에 대해 평활화된 이미지를 이진화하여 객체를 나눠서 mask 변수에 저장합니다.
mask = cv.adaptiveThreshold(equal, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 301, -5)

# 잡음을 제거하기 위해 모폴로지 연산을 적용하고, cut() 함수를 이용하여 붙어있는 객체가 있다면 분리합니다.
# 각 파라미터의 값들은 여러 실험을 통해 제 생각에 좋다고 판단된 값입니다.
structure = cv.getStructuringElement(cv.MORPH_CROSS, (3, 3))
mask = cv.morphologyEx(mask, cv.MORPH_OPEN, structure, iterations=2)
mask = cv.morphologyEx(mask, cv.MORPH_CLOSE, structure, iterations=1)
mask = cut(mask, 10)

cnt, _, stat, _ = cv.connectedComponentsWithStats(mask)
answer = []
for i in range(1, cnt):
    x, y, w, h, area = stat[i]
    # 너비와 높이가 많이 다르거나, 아주 작으면 주사위 객체가 아닌 잡음 객체로 판단하여 무시하도록 합니다.
    if w < 30 or h < 30 or w/h > 1.5 or h/w > 1.5:
        continue

    # 원본 이미지에서 해당 객체가 있는 위치를 tmp 변수가 가리키도록 합니다.
    tmp = src[y:y+h, x:x+w]

    # 원본 이미지의 해당 객체가 있는 위치에서 원을 검출합니다.
    dst = cv.cvtColor(tmp, cv.COLOR_BGR2GRAY)
    circles = cv.HoughCircles(dst, cv.HOUGH_GRADIENT, 1, 25, param1=100, param2=25, minRadius=10, maxRadius=w // 3)

    # 원의 개수를 셀 때 평균 반지름을 기준으로 너무 큰 원은 세지 않도록 합니다.
    if circles is not None:
        count = 0
        for i in range(circles.shape[1]):
            cx, cy, radius = circles[0][i]
            if radius <= circles[0][i].mean()+3:
                count += 1

        answer.append(count)

answer.sort()
print(answer)

cv.waitKey()
cv.destroyAllWindows()