import json
import os
import shutil
import sys
import subprocess
import time


def get_worlds(config):
    worlds = []
    for worldname, worldcfg in config['worlds'].items():
        world = {'name': worldname}

        if 'tilesize' in worldcfg:
            world['tilesize'] = worldcfg['tilesize']
        elif 'tilesize' in config:
            world['tilesize'] = config['tilesize']
        else:
            raise Exception('Could not find tile size for {}.'.format(worldname))

        if 'maptypes' in worldcfg:
            world['maptypes'] = worldcfg['maptypes']
        elif 'maptypes' in config:
            world['maptypes'] = config['maptypes']
        else:
            raise Exception('Could not find map types for {}.'.format(worldname))

        if 'worlddir' in worldcfg:
            world['worlddir'] = worldcfg['worlddir']
        elif 'worlddir' in config:
            world['worlddir'] = os.path.join(config['worlddir'], worldname)
        else:
            raise Exception('Could not find world directory for {}.'.format(worldname))

        if 'wwwdir' in worldcfg:
            world['wwwdir'] = worldcfg['wwwdir']
        elif 'wwwdir' in config:
            world['wwwdir'] = os.path.join(config['wwwdir'], worldname)
        else:
            raise Exception('Could not find www directory for {}.'.format(worldname))

        # if 's3' in worldcfg and worldcfg['s3'] is not False:
        #     world['s3'] = {}
        #     if 'bucket' in worldcfg['s3']:
        #         world['s3']['bucket'] = worldcfg['s3']['bucket']
        #     elif 's3' in config and 'bucket' in config['s3']:
        #         world['s3']['bucket'] = config['s3']['bucket']
        #     else:
        #         raise Exception('Could not find S3 bucket for {}.'.format(worldname))
        #
        #     if 'path' in worldcfg['s3']:
        #         world['s3']['path'] = '/{}'.format(worldcfg['s3']['path'].strip('/'))
        #     elif 's3' in config and 'path' in config['s3']:
        #         world['s3']['path'] = '/{}/{}'.format(config['s3']['path'].strip('/'), worldname)
        #     else:
        #         raise Exception('Could not find S3 path for {}.'.format(worldname))

        worlds.append(world)

    return worlds


def render_world(config, world):
    info = {'apiKey': config['apikey'],
            'tileSize': world['tilesize'],
            'types': [],
            }

    for maptype in world['maptypes']:
        print('Rendering {} map for {}...'.format(maptype['name'], world['name']))

        mapdir = os.path.join(world['wwwdir'], maptype['dir'])

        command = [arg for arg in
                   [os.path.join(config['bindir'], 'cmapbash'),
                    '-d' if maptype['dark'] or maptype['night'] else None,
                    '-i' if maptype['iso'] else None,
                    '-n' if maptype['nether'] or maptype['hell'] else None,
                    '-e' if maptype['end'] else None,
                    '-b',
                    # '-o', imgpath,
                    '-w', world['worlddir'],
                    '-g', mapdir
                    ]
                   if arg]
        print(' '.join(command))

        proc = subprocess.Popen(command, cwd=config['bindir'],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        for line in iter(proc.stdout.readline, ''):
            if not line:
                break
            print('   ' + line.rstrip().decode('utf-8'))
        print('Done rendering.')

        zlevels = sorted(int(zdir[4:]) for zdir in os.listdir(mapdir) if zdir[:4] == 'zoom')

        info['types'].append({
            'id': maptype['dir'],
            'name': maptype['name'],
            'maxZoom': zlevels[-1],
            'minZoom': zlevels[0]
            })

    info['time'] = time.strftime('%B %d, %Y').replace(' 0', ' ')

    # put info.json in www dir
    with open(os.path.join(world['wwwdir'], 'info.json'), 'w') as infofile:
        print('Saving page to {}...'.format(world['wwwdir']))
        json.dump(info, infofile)

    # put google maps page in www dir
    dirname = os.path.dirname(__file__)
    shutil.copy(os.path.join(dirname, 'index.html'), world['wwwdir'])
    shutil.copy(os.path.join(dirname, 'style.css'), world['wwwdir'])
    shutil.copy(os.path.join(dirname, 'script.js'), world['wwwdir'])


if __name__ == '__main__':
    configpath = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), 'config.json')
    with open(configpath, 'rb') as configfile:
        config = json.loads(configfile.read().decode('utf-8'))

    for world in get_worlds(config):
        render_world(config, world)
