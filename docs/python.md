Title: How to use libproxy in Python
Slug: snippets

# How to use libproxy in Python

```
import gi
gi.require_version('Libproxy', '1.0')
from gi.repository import Libproxy
import requests

url = 'https://github.com/libproxy/libproxy'

pf = Libproxy.ProxyFactory()
proxies = pf.get_proxies(url)

success = False
for proxy in proxies:
    response = requests.get(url) #, proxies=proxies)
    
    if response.status_code == 200:
        success = True
        break

if success:
    print(f"The requested URL {url} could be retrieved using the current setup!")
else:
    print(f"The requested URL {url} could *NOT* be retrieved using the current setup")
```

