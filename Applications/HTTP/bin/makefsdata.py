#/usr/bin/env python

from pathlib import Path
import binascii
import textwrap
from css_html_js_minify import html_minify, js_minify, css_minify
import zlib
import bs4

fsdir = Path('../httpd-fs/')
outfile = Path('../src/httpd-fsdata.c')
outfile.touch(exist_ok=True)
extensions = ['.html', '.css', '.js', '.svg', '.shtml', '.json', '.png', '.gif', '.jpg']
unify = True
minify = True
gzip_extensions = ['.html', '.css', '.js', '.svg']

def gzip_encode(content):
    gzip_compress = zlib.compressobj(9, zlib.DEFLATED, zlib.MAX_WBITS | 16)
    data = gzip_compress.compress(content) + gzip_compress.flush()
    return data

def generate_fs():
  files = [p for p in sorted(fsdir.iterdir()) if p.is_file() and p.suffix in extensions]
  if all(file.stat().st_mtime < outfile.stat().st_mtime for file in files):
    print('generated file up to date')
    #return

  with outfile.open('w') as fout:
    fout.write('#include "stddef.h"\n')
    fout.write('#include "httpd-fsdata.h"\n\n')
    fout.write('#define file_NULL (struct fsdata_file *) NULL\n')

    filecount = 0

    for file in files:
      if(unify):
        if(file.suffix not in ['.htm', '.html']):
          continue
        with file.open() as fin:
          html = fin.read()
          if(minify): html = html_minify(html)
          soup = bs4.BeautifulSoup(html, 'html.parser')
          for style in soup.findAll("link", {"rel": "stylesheet"}):
            if(style["href"][0:4] == "http"):
              continue
            css_path = fsdir/style["href"]
            if css_path.is_file():
              with css_path.open() as css_file:
                css = css_file.read()
                if(minify): css = css_minify(css)
                c = bs4.element.NavigableString(css)
                tag = soup.new_tag('style')
                tag.insert(0,c)
                tag['type'] = 'text/css'
                style.replaceWith(tag)
          for script in soup.findAll("script"):
            if not script.has_attr('src'):   continue
            if script['src'][0:4] == 'http': continue
            js_path = fsdir/script["src"]
            if js_path.is_file():
              with js_path.open() as js_file:
                js = js_file.read()
                if(minify): js = js_minify(js)
                j = bs4.element.NavigableString(js)
                tag = soup.new_tag('script')
                tag.insert(0,j)
                script.replaceWith(tag)
          for image in soup.findAll("img"):
            if not image.has_attr('src'):   continue
            if image['src'][0:4] == 'http': continue
            img_path = fsdir/image["src"]
            if img_path.is_file():
              with img_path.open() as img_file:
                img_data = img_file.read()
                if img_path.suffix == '.svg':
                    img_tag = '<img src=\'data:image/svg+xml;utf8,{0}\'>'.format(img_data)
                elif img_path.suffix == '.png':
                    img_data = img_data.encode('base64').replace('\n', '')
                    img_tag = '<img src="data:image/png;base64,{0}">'.format(data_uri)
                tag = bs4.BeautifulSoup(img_tag, 'html.parser')
                image.replaceWith(tag)
          data = soup.decode(formatter="html")
      else:
        with file.open() as fin:
          data = fin.read()
          if(minify):
            if(file.suffix in ['.htm', '.html']):
              data = html_minify(data)
            elif(file.suffix in ['.js']):
              data = js_minify(data)
            elif(file.suffix in ['.css']):
              data = css_minify(data)

      print(file.name)

      filename_enc = file.name.translate(str.maketrans('.-', '__'))
      filename_slash = '/' + file.name
      fout.write('static const unsigned int dummy_align__{} = {};\n'.format(filename_enc, filecount))
      fout.write('static const unsigned char data__{}[] = {{\n'.format(filename_enc))
      fout.write('\n\n')

      fout.write('/* /{} ({} chars) */\n'.format(file.name, len(filename_slash) + 1))
      filename_hex = ",".join("0x{:02x}".format(c) for c in filename_slash.encode()) + ','
      filename_hex += '0x00,' * (4 - (len(filename_slash) % 4))
      fout.write('\n'.join(textwrap.wrap(filename_hex, 80)))
      fout.write('\n\n')

      data = data.encode()
      if(file.suffix in gzip_extensions):
          data = gzip_encode(data)
      data_hex = ",".join("0x{:02x}".format(c) for c in data)
      fout.write('/* raw file data ({} bytes) */\n'.format(len(data)))
      fout.write('\n'.join(textwrap.wrap(data_hex, 80)))
      fout.write(',\n};\n')
      fout.write('\n\n')

      filecount += 1
        
    last_file = ''
    for file in files:
      if(unify):
        if(file.suffix not in ['.htm', '.html']):
          continue
      filename_enc = file.name.translate(str.maketrans('.-', '__'))
      filename_slash = '/' + file.name
      fout.write('const struct fsdata_file file__{}[] = {{ {{\n'.format(filename_enc))
      if last_file == '': fout.write('file_NULL,\n')
      else:               fout.write('file__{},\n'.format(last_file))
      fout.write('data__{},\n'.format(filename_enc))
      fout.write('data__{} + {},\n'.format(filename_enc, len(filename_slash) + (4 - (len(filename_slash) % 4))))
      fout.write('sizeof(data__{}) - {},\n'.format(filename_enc, len(filename_slash) + (4 - (len(filename_slash) % 4))))
      fout.write('#ifdef HTTPD_FS_STATISTICS\n')
      fout.write('#if HTTPD_FS_STATISTICS == 1\n')
      fout.write('0,\n')
      fout.write('#endif /* HTTPD_FS_STATISTICS */\n')
      fout.write('#endif /* HTTPD_FS_STATISTICS */\n')
      fout.write('} };\n')
      fout.write('\n\n')
      last_file = filename_enc

    fout.write('const struct fsdata_file *fs_root = file__{};\n'.format(last_file))
    fout.write('#ifdef HTTPD_FS_STATISTICS\n')
    fout.write('#if HTTPD_FS_STATISTICS == 1\n')
    fout.write('uint16_t fs_count[{}];\n'.format(filecount))
    fout.write('#endif /* HTTPD_FS_STATISTICS */\n')
    fout.write('#endif /* HTTPD_FS_STATISTICS */\n')

generate_fs()
