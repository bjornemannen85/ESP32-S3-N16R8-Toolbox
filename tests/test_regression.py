from pathlib import Path
import re, subprocess, unittest
ROOT=Path(__file__).resolve().parents[1]
BASE='16e766162af5ad983ba4d37cf1021c7dec6bffe6'
def baseline(path):return subprocess.check_output(['git','show',f'{BASE}:{path}'],cwd=ROOT,text=True)
class Regression(unittest.TestCase):
 def test_wifi_setup_and_keyboard_have_valid_titles(self):
  s=(ROOT/'src/main.cpp').read_text()
  pages=[x.strip() for x in re.search(r'enum Page \{([^}]+)',s)[1].split(',') if x.strip()!='PAGE_COUNT']
  names=re.findall(r'"([^"]+)"',re.search(r'const char \*names\[\]=\{([^}]+)',s)[1])
  self.assertEqual(len(pages),len(names))
  self.assertEqual(names[pages.index('WIFI_SETUP')],'WIFI SETUP')
  self.assertEqual(names[pages.index('WIFI_KEYBOARD')],'WIFI KEYBOARD')
 def test_display_touch_and_partition_layout_preserved(self):
  s=(ROOT/'src/main.cpp').read_text();old=baseline('src/main.cpp')
  for line in old.splitlines():
   if line.startswith('constexpr int LCD_') or line.startswith('constexpr int TOUCH_'):
    self.assertIn(line,s)
  self.assertEqual(re.search(r'bool readTouch.*?\n}',s,re.S)[0],re.search(r'bool readTouch.*?\n}',old,re.S)[0])
  path='idf_arduino_nat_probe/partitions.csv'
  self.assertEqual((ROOT/path).read_text(),baseline(path))
 def test_nat_engine_preserved(self):
  old=baseline('src/v61_wifi.h')
  new=(ROOT/'src/v61_wifi.h').read_text()
  start=old.index('static bool enableNat()');end=old.index('static String escapeJson')
  self.assertEqual(old[start:end],new[new.index('static bool enableNat()'):new.index('static String escapeJson')])
 def test_no_legacy_api_removed(self):
  pattern=r's.on\("([^"]+)",\s*(HTTP_GET|HTTP_POST)'
  for p in (ROOT/'src').glob('v61*.h'):
   self.assertEqual(re.findall(pattern,p.read_text()),re.findall(pattern,baseline('src/'+p.name)))
 def test_firmware_packaging_is_separate_and_keeps_nat(self):
  s=(ROOT/'.github/workflows/v62-full-access.yml').read_text()
  self.assertIn('branches: [v6.2-full-access]',s)
  self.assertIn('esp_idf_version: v5.3.2',s)
  self.assertIn('cyberdeck-v6.2-full-access-nat-firmware',s)
  for path in ['.github/workflows/build.yml','.github/workflows/arduino-idf-nat-probe.yml','idf_arduino_nat_probe/ci/package.sh']:
   self.assertEqual((ROOT/path).read_text(),baseline(path))
 def test_catalog_current_and_parameter_count_fits_touch(self):
  subprocess.run(['python','ci/catalog.py','--check'],cwd=ROOT,check=True)
  s=(ROOT/'src/v62_parameters.h').read_text()
  entries=re.findall(r'\{"([^"]+)",(HTTP_\w+),"([^"]*)"\}',s)
  self.assertGreaterEqual(len(entries),90)
  for path,method,fields in entries:
   self.assertLessEqual(len(fields.split(',')) if fields else 0,4,(path,fields))
  for required in ['/wifi/nat','/api/v61/uart/start','/scripts/save','/api/v61/ble-transfer/start','/api/v61/sd/mount']:
   self.assertIn(required,[x[0] for x in entries])
if __name__=='__main__':unittest.main()
