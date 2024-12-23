from PIL import Image
from glob import glob


MAIN_ALPHA = int(255 * 0.75)
OUTLINE_ALPHA = int(255 * 0.9)

OUT_DIR = 'output'
PRESET_DIR = 'presets'


COLORS = (
    ('FICSIT', (119, 121, 139), (54, 55, 63)),
    ('Metal', (156, 156, 176), (88, 88, 99)),
    ('Concrete', (213, 213, 216), (137, 137, 139)),
    ('CoatedConcrete', (176, 176, 189), (158, 156, 152)),
    ('Asphalt', (74, 75, 84), (127, 127, 122))
)

FRAME = ('Frame', (134, 133, 148), (42, 42, 47))
GLASS = ('Glass', (236, 236, 236), (134, 133, 148))


def generate_image(img_path, color):
    img = Image.open(img_path)
    w, h = img.size
    _, main, outline = color
    new_img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    for i in range(w):
        for j in range(h):
            color = img.getpixel((i, j))
            if color == (0, 0, 0, 255) or color == (0, 0, 0):
                new_img.putpixel((i, j), (*main, MAIN_ALPHA))
            elif color == (255, 255, 255, 255) or color == (255, 255, 255):
                new_img.putpixel((i, j), (*outline, OUTLINE_ALPHA))
    return new_img


def generate_foundations():
    presets = glob(f'{PRESET_DIR}/*.png')

    for preset in presets:
        preset_name = preset.split('\\')[-1].split('/')[-1].split('.')[0]
        if preset_name in ('Foundation_Plus',):
            continue

        for color in COLORS:
            name, _, _ = color
            new_img = generate_image(preset, color)
            new_img.save(f'{OUT_DIR}/{name}/{preset_name}_{name}.png')


def generate_foundations_extra():
    default = f'{PRESET_DIR}/Foundation_Default.png'
    plus = f'{PRESET_DIR}/Foundation_Plus.png'
    line = f'{PRESET_DIR}/Foundation_MT_MB_Line.png'

    generate_image(default, FRAME).save(f'{OUT_DIR}/Frame/Foundation_Default_Frame.png')
    generate_image(plus, FRAME).save(f'{OUT_DIR}/Frame/Foundation_Plus_Frame.png')
    generate_image(line, GLASS).save(f'{OUT_DIR}/Glass/Foundation_MT_MB_Line_Glass.png')


if __name__ == '__main__':
    generate_foundations_extra()
