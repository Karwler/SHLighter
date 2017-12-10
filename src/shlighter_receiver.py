#!/usr/bin/python3

from sense_hat import SenseHat
import signal
import socket
import time

# command
MOVE_LINE_UP = 0
MOVE_LINE_RIGHT = 1
MOVE_LINE_DOWN = 2
MOVE_LINE_LEFT = 3

MOVE_FULL_UP = 4
MOVE_FULL_RIGHT = 5
MOVE_FULL_DOWN = 6
MOVE_FULL_LEFT = 7

MOVE_STEP_UP = 8
MOVE_STEP_RIGHT = 9
MOVE_STEP_DOWN = 10
MOVE_STEP_LEFT = 11

MAKE_DOTS_0 = 12
MAKE_DOTS_1 = 13
MAKE_DOTS_2 = 14
MAKE_DOTS_3 = 15

MOVE_IN_0 = 16
MOVE_IN_1 = 17

# directions
UP = 0
RIGHT = 1
DOWN = 2
LEFT = 3

# defaults
DLEN = 4
PORT = 51721
XMIN = 1
XMAX = 7
YMIN = 0
YMAX = 7
DELAY = 0.1

# variables
sh = SenseHat()
run = True

def setColumn(x, color):
	for y in range(YMIN, YMAX+1):
		sh.set_pixel(x, y, color)

def setRow(y, color):
	for x in range(XMIN, XMAX+1):
		sh.set_pixel(x, y, color)

def setRect(xp, xe, yp, ye, color):
	for x in range(xp, xe, 1):
		sh.set_pixel(x, yp, color)
	for y in range(yp, ye, 1):
		sh.set_pixel(xe, y, color)
	for x in range(xe, xp, -1):
		sh.set_pixel(x, ye, color)
	for y in range(ye, yp, -1):
		sh.set_pixel(xp, y, color)

def getMove(direction, step):
	if direction == UP:
		return range(YMAX, YMIN-1, -step)
	if direction == RIGHT:
		return range(XMIN, XMAX+1, step)
	if direction == DOWN:
		return range(YMIN, YMAX+1, step)
	if direction == LEFT:
		return range(XMAX, XMIN-1, -step)
	return []

def moveLine(direction, color):
	mov = getMove(direction, 1)
	vert = direction == UP or direction == DOWN
	func = setRow if vert else setColumn
	bar = range(XMIN, XMAX+1) if vert else range(YMIN, YMAX+1)

	for m in mov:
		orig = []
		for b in bar:
			if vert:
				orig.append(sh.get_pixel(b, m))
			else:
				orig.append(sh.get_pixel(m, b))

		func(m, color)
		time.sleep(DELAY)

		i = 0
		for b in bar:
			if vert:
				sh.set_pixel(b, m, orig[i])
			else:
				sh.set_pixel(m, b, orig[i])
			i += 1

def moveFull(direction, color, step):
	mov = getMove(direction, step)
	func = setRow if direction == UP or direction == DOWN else setColumn

	for m in mov:
		func(m, color)
		time.sleep(DELAY)

def makeDots(index, color):
	xmin = XMIN+1 if index == 1 or index == 3 else XMIN
	ymin = YMIN+1 if index == 2 or index == 3 else YMIN

	for y in range(ymin, YMAX+1, 2):
		for x in range(xmin, XMAX+1, 2):
			sh.set_pixel(x, y, color)
			time.sleep(DELAY/4)

def moveIn(ofs, color):
	xp = XMIN + ofs
	xe = XMAX - ofs
	yp = YMIN + ofs
	ye = YMAX - ofs

	xs = XMAX - XMIN
	ys = YMAX - YMIN
	ie = xs if xs < ys else ys
	for i in range(1, int(ie/2)):
		setRect(xp, xe, yp, ye, color)
		xp += 2
		xe -= 2
		yp += 2
		ye -= 2
		time.sleep(DELAY)

def clear():
	for x in range(XMIN, XMAX+1):
		for y in range(YMIN, YMAX+1):
			sh.set_pixel(x, y, (0, 0, 0))

def sigTerm(signal, frame):
    global run
    run = False

# init
signal.signal(signal.SIGTERM, sigTerm)
signal.signal(signal.SIGINT, sigTerm)

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(("", PORT))
server.listen(1)
(client, addr) = server.accept()

clear()

# main
while run:
	data = client.recv(DLEN)
	if not data:
		break
	else:
		color = (data[1], data[2], data[3])
		if data[0] >= MOVE_LINE_UP and data[0] <= MOVE_LINE_LEFT:
			moveLine(data[0] - MOVE_LINE_UP, color)
		elif data[0] >= MOVE_FULL_UP and data[0] <= MOVE_FULL_LEFT:
			moveFull(data[0] - MOVE_FULL_UP, color, 1)
		elif data[0] >= MOVE_STEP_UP and data[0] <= MOVE_STEP_LEFT:
			moveFull(data[0] - MOVE_STEP_UP, color, 2)
		elif data[0] >= MAKE_DOTS_0 and data[0] <= MAKE_DOTS_3:
			makeDots(data[0] - MAKE_DOTS_0, color)
		elif data[0] >= MOVE_IN_0 and data[0] <= MOVE_IN_1:
			moveIn(data[0] - MOVE_IN_0, color)

clear()
