from pathlib import Path
import sys,subprocess,tempfile,json,hashlib
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
subprocess.run([sys.executable,str(root/'tools/audit-jitter-native.py'),sys.argv[1]],check=True)
with tempfile.TemporaryDirectory() as tmp:
 exe=str(Path(tmp)/'native-multiply')
 subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(here/'execute.cpp'),'-o',exe],check=True)
 result=subprocess.check_output([exe],text=True).strip()
files=[here/'execute.cpp',here/'native-multiply.h',root/'src/shaders/rr_jitter_ray.hlsli',root/'tools/audit-jitter-native.py']
(here/'results.json').write_text(json.dumps({'status':'LOCAL_NATIVE_ARITHMETIC_PASS','result':result,'sanitizer_limit':'Extracted machine instructions are not sanitizer-instrumented. Shared shader arithmetic is covered separately by normal/ASan/UBSan tests.','gpu_execution':'NOT_RUN','sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}},indent=2)+'\n');print(result)
