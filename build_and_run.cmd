cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build

copy build\clidll\Debug\pict.dll build\clidll-usage\Debug\pict.dll

pushd build\api-usage\Debug && call pictapisamples.exe && popd
pushd build\clidll-usage\Debug && call pictclidllusage.exe && popd