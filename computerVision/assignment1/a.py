import numpy as np
import cv2 as cv

def a1():
    src = cv.imread('sample.jpg', cv.IMREAD_GRAYSCALE)

    if src is None:
        print('Image load failed!')
        exit()

    # 평균 밝기보다 작은 픽셀들은 전부 0으로 만듭니다.
    src[src < src.mean()] = 0

    # output.jpg 파일로 저장합니다.
    cv.imwrite('output.jpg', src)

def a2():
    src = cv.imread('sample.jpg', cv.IMREAD_GRAYSCALE)

    if src is None:
        print('Image load failed!')
        exit()

    des = np.zeros(src.shape, np.int16)

    # 계수는 2.0으로 하고 평균 밝기를 기준으로 명암비를 조절합니다.
    des[:] = src * 2.0 - src.mean()

    # saturation 연산을 하여 255 이상인 값은 255로, 0 이하인 값은 0으로 만듭니다.
    des[des > 255] = 255
    des[des < 0] = 0

    # contrast.jpg 파일로 저장합니다.
    cv.imwrite('contrast.jpg', des)

def a3():
    # 카메라로부터 영상을 받아옵니다.
    cap = cv.VideoCapture(0)
    if not cap.isOpened():
        exit()

    # 프레임의 높이와 너비와 초당 프레임 수를 불러와서 delay를 계산합니다.
    w = round(cap.get(cv.CAP_PROP_FRAME_WIDTH))
    h = round(cap.get(cv.CAP_PROP_FRAME_HEIGHT))
    fps = round(cap.get(cv.CAP_PROP_FPS))
    delay = round(1000 / fps)

    # DIVX 코덱을 사용합니다.
    fourcc = cv.VideoWriter_fourcc(*'DIVX')

    # output.avi 파일로 동영상을 저장합니다.
    outputVideo = cv.VideoWriter('output.avi', fourcc, fps, (w, h))
    if not outputVideo.isOpened():
        exit()

    # 카메라로부터 영상을 읽어서 영상의 평균 밝기를 계산합니다.
    ref, img = cap.read()
    nowMean = img.mean()

    frame = 0   # 3초를 세기 위한 프레임 개수
    fps3 = fps * 3  # 3초당 프레임 수

    flag = False    # flag가 True이면 반전을 수행합니다.

    while True:
        if not ref:
            break

        # 다음 이미지를 읽어옵니다.
        ref, nextImg = cap.read()

        # fps3 만큼 프레임이 지나면 3초가 지난 것이고 flag를 False로 바꿔줘서 이미지 반전을 그만합니다.
        frame += 1
        if frame > fps3:
            flag = False
        if flag:
            img = ~img

        # 다음 프레임과 평균 밝기가 30 이상 차이나면
        # 3초를 세기 위해 프레임을 세기 시작합니다.
        # 그리고 flag를 True로 바꿔줘서 이미지 반전을 합니다.
        if abs(nowMean - nextImg.mean()) > 30:
            frame = 0
            flag = True

        # 영상을 저장합니다.
        outputVideo.write(img)
        cv.imshow('img', img)

        img = nextImg
        nowMean = img.mean()

        if cv.waitKey(delay) == 27:
            break

    cap.release()
    outputVideo.release()
    cv.destroyAllWindows()

a1()
a2()
a3()
