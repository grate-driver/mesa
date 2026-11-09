#!/usr/bin/env python3
"""CPU-only test for the R16 research snapshot; no graphics device is opened."""
import argparse,json,pathlib,shlex,subprocess
p=argparse.ArgumentParser()
for n in ['build-dir','driver-source','grate-archive','sysroot','qemu','runtime-lib']:
 p.add_argument('--'+n,type=pathlib.Path,required=True)
p.add_argument('--ninja',default='ninja');p.add_argument('--sampler',action='store_true');p.add_argument('--bias',action='store_true');a=p.parse_args()
if a.sampler and a.bias:p.error('Select sampler or bias, not both')
build=a.build_dir.resolve();driver=a.driver_source.resolve();lab=pathlib.Path(__file__).resolve().parent
out=build/('grate-compat-sampler-cpu' if a.sampler else 'grate-lod-bias-cpu' if a.bias else 'grate-compat-cpu');out.mkdir(exist_ok=True)
e=next(e for e in json.loads((build/'compile_commands.json').read_text()) if pathlib.Path(e['file']).name=='grate_program.c')
args=shlex.split(e['command']);compile=[];i=0
while i<len(args):
 x=args[i]
 if x in ['-o','-MF','-MQ']:i+=2;continue
 if x=='-MD':i+=1;continue
 if x==e['file']:x=str(lab/('sampler-regression.c' if a.sampler else 'lod-bias-regression.c' if a.bias else 'compat-regression.c'))
 x=x.replace('../source/src/gallium/drivers/grate',str(driver))
 compile.append(x);i+=1
compile+=['-I'+str(driver),'-o',str(out/'regression.o')]
subprocess.run(compile,cwd=build,check=True)
cmds=subprocess.check_output([a.ninja,'-C',str(build),'-t','commands','src/gallium/targets/dri/libgallium_dri.so'],text=True)
args=shlex.split(cmds.splitlines()[-1]);link=[];i=0
while i<len(args):
 x=args[i]
 if x in ['-Wl,--version-script','-Wl,--dynamic-list']:i+=2;continue
 if x=='-shared' or x.startswith(('-Wl,-soname,','-Wl,-rpath,')):i+=1;continue
 if x=='src/gallium/targets/dri/libgallium_dri.so.p/target.c.o':x=str(out/'regression.o')
 if x=='src/gallium/drivers/grate/libgrate.a':x=str(a.grate_archive.resolve())
 link.append(x);i+=1
link[link.index('-o')+1]=str(out/'regression')
subprocess.run(link,cwd=build,check=True)
run=subprocess.run([str(a.qemu.resolve()),'-L',str(a.sysroot.resolve()),'-E','LD_LIBRARY_PATH='+str(a.runtime_lib.resolve())+':'+str(a.sysroot.resolve()/'usr/lib'),str(out/'regression')],capture_output=True,text=True,timeout=30)
result={'exit':run.returncode,'stdout':run.stdout,'stderr':run.stderr}
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(run.returncode)
