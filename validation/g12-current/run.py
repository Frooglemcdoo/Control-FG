from pathlib import Path
import json, os, subprocess, hashlib
root=Path(__file__).resolve().parents[2]
here=Path(__file__).resolve().parent
normal=here/'reflectance-test'
san=here/'reflectance-test-sanitized'
commands=[
 ['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'reflectance-test.cpp'),'-o',str(normal)],
 ['g++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(here/'reflectance-test.cpp'),'-o',str(san)]]
outputs=[]
try:
 for command,binary in zip(commands,(normal,san)):
  subprocess.run(command,check=True,cwd=root)
  result=subprocess.run([str(binary)],check=True,cwd=root,text=True,capture_output=True,
                        env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  outputs.append(result.stdout)
 finally_status='PASS'
finally:
 for binary in (normal,san):
  binary.unlink(missing_ok=True)
report={'schema':'ControlFG.RRNativeG12.ReflectanceTests.v1','status':finally_status,
        'normal':'PASS','asan_ubsan':'PASS','assertions':outputs[0].strip().splitlines(),
        'independent_nvidia_golden_vectors':0,
        'matrix_reference_cases':101505,
        'reference_url':'https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md#421-specular-albedo-generation',
        'reference_method':'double matrix expression versus expanded float implementation; not GPU validation',
        'leak_sanitizer':'disabled: runtime ptrace incompatibility; ASan and UBSan remain enabled',
        'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'src/rr_reflectance.h',here/'reflectance-test.cpp')},
        'rr_evaluation_enabled':False}
(here/'reflectance-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: G12 normal and ASan/UBSan reflectance tests')
