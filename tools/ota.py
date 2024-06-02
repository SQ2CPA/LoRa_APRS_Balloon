import requests
import sys

env = sys.argv[1]
host = sys.argv[2]

base_url = f'http://{host}'

response = requests.get(f'{base_url}/ota/start?mode=fr&hash=441018525208457705bf09a8ee3c1093')

if response.text == 'OK':
    print('OTA flashing started. Uploading..')

    bin_path = 'ota_update.bin'

    with open(bin_path, 'rb') as file:
        files = {'file': file}

        response = requests.post(f'{base_url}/ota/upload', files=files)

        if response.text == 'OK':
            print("Binary uploaded. Rebooting..")

            try:
                requests.post(f'{base_url}/action?type=reboot', timeout=1)
            except requests.exceptions.Timeout:
                pass
        
            print("Flashing done.")
        else:
            print('OTA flashing failed! (2)')
            print(response.text)
else:
    print('OTA flashing failed! (1)')
    print(response.text)
