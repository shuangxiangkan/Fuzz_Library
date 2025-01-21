import os
import struct
import random

def generate_minimal_j2k(output_path):
    """生成一个最小的合法J2K文件"""
    # J2K文件的基本头部
    header = bytes([
        0xFF, 0x4F,  # SOC (Start of Codestream) marker
        0xFF, 0x51,  # SIZ marker (Image and tile size)
        0x00, 0x0B,  # Length of marker segment
        0x00, 0x01,  # Precision
        0x00, 0x01,  # Height
        0x00, 0x01,  # Width
        0x00, 0x01,  # Components
        0x00, 0x00,  # Unused bits
        0xFF, 0x52   # COD (Coding style default) marker
    ])
    
    with open(output_path, 'wb') as f:
        f.write(header)

def generate_minimal_jp2(output_path):
    """生成一个最小的合法JP2文件"""
    # JP2文件的基本结构
    # JPEG 2000 signature box
    signature = b'\x00\x00\x00\x0C\x6a\x50\x20\x20\x0D\x0A\x87\x0A'
    
    # File type box
    ftyp = b'\x00\x00\x00\x14\x66\x74\x79\x70\x6A\x70\x32\x20\x00\x00\x00\x00\x6A\x70\x32\x20'
    
    # Header box (minimal)
    ihdr = b'\x00\x00\x00\x0E\x69\x68\x64\x72\x00\x01\x00\x01\x01'
    
    with open(output_path, 'wb') as f:
        f.write(signature)
        f.write(ftyp)
        f.write(ihdr)

def generate_extreme_variants():
    """生成一些极端但合法的变体"""
    variants = [
        # 单像素，各种颜色深度
        {'width': 1, 'height': 1, 'depth': 1},
        {'width': 1, 'height': 1, 'depth': 8},
        {'width': 1, 'height': 1, 'depth': 16},
        
        # 极小尺寸，不同颜色通道
        {'width': 2, 'height': 2, 'depth': 8},
        
        # 极端长宽比
        {'width': 1, 'height': 256, 'depth': 8},
        {'width': 256, 'height': 1, 'depth': 8}
    ]
    
    return variants

def generate_seed_files(output_dir='seeds'):
    """生成种子文件"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 生成基本的J2K和JP2文件
    generate_minimal_j2k(os.path.join(output_dir, 'minimal_j2k.j2k'))
    generate_minimal_jp2(os.path.join(output_dir, 'minimal_jp2.jp2'))
    
    # 生成极端变体
    variants = generate_extreme_variants()
    
    for i, variant in enumerate(variants, 1):
        # J2K极端变体
        j2k_path = os.path.join(output_dir, f'extreme_j2k_{i}.j2k')
        jp2_path = os.path.join(output_dir, f'extreme_jp2_{i}.jp2')
        
        generate_custom_j2k(j2k_path, variant)
        generate_custom_jp2(jp2_path, variant)
    
    print(f"生成了 {len(variants) * 2 + 2} 个种子文件在 {output_dir} 目录")

def generate_custom_j2k(output_path, variant):
    """根据变体生成自定义J2K文件"""
    def int_to_bytes(x):
        return x.to_bytes(2, byteorder='big')
    
    # SOC marker
    header = bytes([0xFF, 0x4F])
    
    # SIZ marker
    header += b'\xFF\x51'
    header += int_to_bytes(11)  # Marker length
    header += int_to_bytes(variant['depth'])  # Precision
    header += int_to_bytes(variant['height'])  # Height
    header += int_to_bytes(variant['width'])   # Width
    header += b'\x00\x01'  # Number of components
    header += b'\x00\x00'  # Unused bits
    
    # COD marker (minimal coding style)
    header += bytes([0xFF, 0x52, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00])
    
    with open(output_path, 'wb') as f:
        f.write(header)

def generate_custom_jp2(output_path, variant):
    """根据变体生成自定义JP2文件"""
    def int_to_bytes(x):
        return x.to_bytes(4, byteorder='big')
    
    # JPEG 2000 signature
    signature = b'\x00\x00\x00\x0C\x6a\x50\x20\x20\x0D\x0A\x87\x0A'
    
    # File type box
    ftyp = b'\x00\x00\x00\x14\x66\x74\x79\x70\x6A\x70\x32\x20\x00\x00\x00\x00\x6A\x70\x32\x20'
    
    # Header box with custom dimensions
    ihdr = b'\x00\x00\x00\x0E\x69\x68\x64\x72'
    ihdr += int_to_bytes(variant['height'])
    ihdr += int_to_bytes(variant['width'])
    ihdr += bytes([0x01])  # Number of components
    
    with open(output_path, 'wb') as f:
        f.write(signature)
        f.write(ftyp)
        f.write(ihdr)

if __name__ == '__main__':
    generate_seed_files()