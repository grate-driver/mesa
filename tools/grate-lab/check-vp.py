#!/usr/bin/env python3
"""Build/run CPU-only ARM shader regression using an existing Meson cross build."""
import argparse,json,os,pathlib,shlex,subprocess
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=pathlib.Path,required=True);p.add_argument('--sysroot',type=pathlib.Path,required=True);p.add_argument('--qemu',type=pathlib.Path,required=True);p.add_argument('--runtime-lib',type=pathlib.Path,required=True);p.add_argument('--grate-archive',type=pathlib.Path);p.add_argument('--ninja',default='ninja');p.add_argument('--uniforms',action='store_true');p.add_argument('--sampler',action='store_true');p.add_argument('--resources',action='store_true');a=p.parse_args()
if sum([a.uniforms,a.sampler,a.resources])>1:p.error('Select only one CPU regression mode')
lab=pathlib.Path(__file__).resolve().parent;source=lab.parents[1];build=a.build_dir.resolve();out=build/('grate-resource-cpu-regression' if a.resources else 'grate-sampler-cpu-regression' if a.sampler else 'grate-uniform-cpu-regression' if a.uniforms else 'grate-cpu-regression');out.mkdir(exist_ok=True)
db=json.loads((build/'compile_commands.json').read_text());e=next(e for e in db if pathlib.Path(e['file']).name=='grate_program.c');args=shlex.split(e['command']);clean=[];i=0
while i<len(args):
 x=args[i]
 if x in ['-o','-MF','-MQ']:i+=2;continue
 if x=='-MD':i+=1;continue
 if x==e['file']:x=str(lab/('mipmap-resource-regression.c' if a.resources else 'sampler-regression.c' if a.sampler else 'uniform-regression.c' if a.uniforms else 'vp-regression.c'))
 x=x.replace('../source/src/gallium/drivers/grate',str(source/'src/gallium/drivers/grate'))
 clean.append(x);i+=1
clean+=['-I'+str(source/'src/gallium/drivers/grate'),'-o',str(out/'regression.o')]
subprocess.run(clean,cwd=build,check=True)
commands=subprocess.check_output([a.ninja,'-C',str(build),'-t','commands','src/gallium/targets/dri/libgallium_dri.so'],text=True);args=shlex.split(commands.splitlines()[-1]);link=[];i=0
while i<len(args):
 x=args[i]
 if x in ['-Wl,--version-script','-Wl,--dynamic-list']:i+=2;continue
 if x=='-shared' or x.startswith(('-Wl,-soname,','-Wl,-rpath,')):i+=1;continue
 if x=='src/gallium/targets/dri/libgallium_dri.so.p/target.c.o':x=str(out/'regression.o')
 if x=='src/gallium/drivers/grate/libgrate.a' and a.grate_archive:x=str(a.grate_archive.resolve())
 link.append(x);i+=1
link[link.index('-o')+1]=str(out/'regression')
if a.resources:link += ['-Wl,--wrap='+n for n in ['drm_tegra_bo_new','drm_tegra_bo_map','drm_tegra_bo_unmap','drm_tegra_bo_unref','drm_tegra_bo_get_size','drm_tegra_bo_get_handle']]
subprocess.run(link,cwd=build,check=True)
results=[]
cases=[None] if (a.uniforms or a.sampler or a.resources) else [lab/'passthrough.tgsi']+sorted(lab.glob('captured-*.tgsi'))
for shader in cases:
 command=[str(a.qemu.resolve()),'-L',str(a.sysroot.resolve()),'-E','LD_LIBRARY_PATH='+str(a.runtime_lib.resolve())+':'+str(a.sysroot.resolve()/'usr/lib'),str(out/'regression')]
 if shader is not None:command.append(str(shader))
 r=subprocess.run(command,capture_output=True,text=True,timeout=30)
 results.append({'shader':shader.name if shader else 'mipmap-resources' if a.resources else 'sampler-state' if a.sampler else 'fragment-uniforms','exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results,indent=2));raise SystemExit(any(x['exit'] for x in results))
