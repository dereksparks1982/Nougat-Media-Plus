#!/usr/bin/env python3
from pathlib import Path
import sys, json
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
desktop=(root/'com.elderredsoftworks.NougatMediaSuite.desktop').read_text(encoding='utf-8')
gitignore=(root/'.gitignore').read_text(encoding='utf-8')
lock=json.loads((root/'components/drone/DRONE_STACK_LOCK.json').read_text(encoding='utf-8'))
checks={
 'version':'Nougat Media Suite v0.0.60' in main and 'VERSION 0.0.60' in cmake,
 'target':'Nougat_Media_Suite_v60' in cmake and 'Nougat_Media_Suite_v60' in desktop,
 'tab':'NOUGAT_V60_DRONE_STUDIO_TAB' in main and 'button_on(target,studioDroneTab,"Drone")' in main,
 'simulation_only':'SIMULATION-ONLY FOUNDATION' in main,
 'director':'"DIRECTOR SHOT"' in main and 'Describe -> Path -> Preview -> Simulate -> Save -> Authorized Flight (later)' in main,
 'telemetry':'TELEMETRY / CAMERA / GIMBAL PIPELINE' in main,
 'ignore':'components/drone/vendor/' in gitignore,
 'pins':lock['mavsdk']['ref']=='v3.17.4' and lock['px4']['ref']=='v1.17.0' and lock['ardupilot']['ref']=='Copter-4.7.0'
}
failed=[k for k,v in checks.items() if not v]
if failed:
    print('FAIL: v0.0.60 Drone foundation contracts:', ', '.join(failed)); raise SystemExit(1)
print('PASS: v0.0.60 Studio Drone foundation contracts')
