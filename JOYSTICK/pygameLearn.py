import pygame
import serial

pygame.init()

win=pygame.display.set_mode((100,100))
pygame.display.set_caption("Pygame")

ser=serial.Serial("COM3",9600)
run=True
while run:
    for even in pygame.event.get():
        if even.type==pygame.QUIT:
            run=False
    data=ser.readline().decode().strip().split(',')
    if len(data)==3:
        x,y,btnState=map(int,data)
    
    win.fill((0,0,0))
    pygame.display().flip()
ser.close()
pygame.quit()