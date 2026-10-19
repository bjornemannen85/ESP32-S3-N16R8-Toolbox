from pathlib import Path
import hashlib,json,re,unittest
from PIL import Image,ImageOps
ROOT=Path(__file__).resolve().parents[1]
A=ROOT/'assets/wolfdeck'
class WolfDeckArtwork(unittest.TestCase):
 def setUp(self):
  self.manifest=json.loads((A/'manifest.json').read_text())
  self.header=(ROOT/'src/v62_wolfdeck_assets.h').read_text()
 def values(self,name):
  body=re.search(r'\b'+name+r'\[\] PROGMEM=\{(.*?)\};',self.header,re.S)[1]
  return [int(x,16) for x in re.findall(r'0x([0-9a-f]+)',body)]
 def test_exact_source_artwork_and_display_geometry(self):
  for role in ['display','boot']:
   path=A/'source'/f'wolfdeck_{role}_320x240.jpg'
   self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),self.manifest[role]['source_sha256'])
   with Image.open(path) as image:self.assertEqual(image.size,(320,240))
   self.assertEqual(len(self.values(role+'Pixels')),76800)
   self.assertEqual(len(self.values(role+'Palette')),256)
 def test_complete_composition_fits_without_crop_or_stretch(self):
  for role in ['display','boot']:
   self.assertEqual(self.manifest[role]['fit'],'contain')
   self.assertEqual(self.manifest[role]['image_rect'],[0,20,320,200])
   palette=self.values(role+'Palette');pixels=self.values(role+'Pixels')
   # All four picture edges remain inside the screen; the bars stay dark.
   for y in list(range(20))+list(range(220,240)):
    self.assertTrue(all(palette[pixels[y*320+x]]<=2 for x in range(320)))
 def test_web_image_compression_and_flash_payload(self):
  path=A/'web/wolfdeck_web_background.jpg';data=path.read_bytes()
  with Image.open(path) as im:
   self.assertEqual(im.size,(1280,800));self.assertEqual(im.format,'JPEG')
  self.assertEqual(bytes(self.values('webJpg')),data)
  self.assertEqual(hashlib.sha256(data).hexdigest(),self.manifest['web']['sha256'])
  self.assertLess(len(data),.65*(A/'source/wolfdeck_web_background.jpg').stat().st_size)
 def test_flash_budget_and_no_framebuffer(self):
  self.assertLess(sum(self.manifest[r]['flash_bytes'] for r in ['display','boot','web']),350000)
  self.assertEqual(self.manifest['runtime_row_buffer_bytes'],640)
 def test_palette_preserves_image_detail(self):
  # Compare RGB565 palette output with the supplied JPEG, before UI dimming.
  for role in ['display','boot']:
   palette=self.values(role+'Palette');pixels=self.values(role+'Pixels')
   decoded=[((palette[p]>>11)*255//31,((palette[p]>>5)&63)*255//63,(palette[p]&31)*255//31) for p in pixels]
   with Image.open(A/'source/wolfdeck_web_background.jpg') as source:
    fitted=ImageOps.contain(source.convert('RGB'),(320,240),Image.Resampling.LANCZOS)
   im=Image.new('RGB',(320,240),(0,0,8));im.paste(fitted,(0,20))
   original=list(im.get_flattened_data())
   error=sum(abs(a-b) for rgb,out in zip(original,decoded) for a,b in zip(rgb,out))/(76800*3)
   self.assertLess(error,5,role)
if __name__=='__main__':unittest.main()
