from PIL import Image
img = Image.open('../assets/textures/poison_splatter.png')

pixels = img.load()

for y in range(img.height):
    for x in range(img.width):
        r,g,b,a = pixels[x,y]
        pixels[x,y] = (0, 255 - g, 0, a)

img.save("result.png") 

