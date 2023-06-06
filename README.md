# DX12 FBX ModelViwer ToyProject

DirectX12 공부를 위한 토이 프로젝트입니다.


##추가종속성

*FBX SDK 2020.2.21
*Microsoft.Direct3D.D3D12
*DirectXTK12_desktop_2019


##개발환경

*Visual Studio 2019
*AMD Ryzen 7 2700 Eight-Core Processor 3.20 GHz
*64bit Processor
*64Bit Windows 10
*RAM 16.0GB
*NVIDIA GeForce GTX 1060 3GB

##구현 기능

*FBX Import
*Mesh Render
*Camera이동
*다양한 파라미터 조정가능
*SceneObject Hierachy 설정가능
*Instancing
*Skeletal Animation 재생

##프로젝트 주요 클래스

1.D3DresourceManager
    *ID3D12Device,ID3D12CommandQueue 같은 DirectX12 API관련 자원을 가지고있는 클래스 

2.D3DModelViewerApp
    *MainWindow관련 클래스

3.FBXModelScene
    *FBX SDK를 이용해 FBX모델을 Import하는 클래스

4.Scene , SceneObject
    *Mesh등의 Scene구성 요소를 트리구조로 가지고있는 클래스

5.MeshObject
    *Render하기위한 데이터를 가지고있는 클래스


##프로젝트 진행도중 발생한 문제와 해결과정

1. 중복 정점 제거 알고리즘

아래 알고리즘을 사용하여 FBXImport시 중복 정점을 인덱스로 분리하는데 많은 시간이 소요

    1. Mesh정점을 얻어 Vector에 저장
    2. 1에서 얻은 Vector를 순회하며 중복이제거된 정점배열에 존재하는지 확인
    3. 존재한다면 해당 인덱스를 인덱스배열에 추가
    4. 존재하지않는다면 중복제거 배열에 정점을 추가하고 해당 인덱스를 인덱스배열에 추가
    5. 2,3,4를 반복

알고리즘 실행 시간이 정점배열크기 * 중복제거 정점배열크기에 비례하여 많은 시간이 소모됨

해결방법 : 중복제거 정점을 HashMap에 저장하여 정점배열크기에 비례한 시간으로 감소


2. DescriptorHeap 할당

D3D12에서 다른 DescriptorHeap을 대상으로 SetDescriptorHeap호출 시 성능하락
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-setdescriptorheaps

따라서 DescriptorHeap을 하나만 할당할 필요가 있음
해결방법: 가변크기메모리풀을 만들어 청크단위로 할당하고 내부에서 작은단위로 할당

참고자료 : http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/


3. 멀티스레드 동기화 문제

FBX Import시 긴 시간동안 UI가 Block됨

스레드를 분리하여 해결할 수 있으나 UI로직 에서 컨테이너를 순회하고 있기 때문에 자료 추가,삭제 시 iterator무효되어 에러발생

해결방법: 비동기로 Import하되 컨테이너 추가 작업을 저장하는 JobQueue클래스를 생성해 메인 스레드에서 처리하도록 함


4. 멀티스레드 성능 문제

Render시 renderlist를 스레드 크기로 분리하여 처리하는데 스레드 생성에 많은 시간을 소요

해결 방법: Pool을 사용하는 std::Async을 사용해 해결

비동기Import 또한 async를 사용하려 했으나 반환 값Future와 수명을 같이 하기 때문에 적합하지 않음

직접 스레드풀을 만들어 해결


5. 디버깅 문제

디버깅 레이어 활성화를 통해 많은 정보를 얻을 수 있으나 디버깅을 하기에 부족함

Visual Studio의 Break Point와 같은 기능이 필요

Visual Studio에 내장된 그래픽 디버거는 특정 버전 이후로 DX12의 지원 되지 않음

PIX가 크게 도움이됨 캡쳐한 프레임의 CommandList이벤트, Shader Input Output, Binding Resources등 많은 정보를 볼 수 있고 Shader에 BreakPoint를 걸면서 확인할 수 있음

관련 Dll로드시 D3D12Device생성전에 로드해야함



##해결하지 못한 문제

루트시그니처 생성,파이프라인 생성,셰이더 리소스 바인딩 등을 하드코딩하였음

셰이더 리플렉션 기능을 통해 Input Element등을 자동설정 할 수 있지만

적절한 RootSignature를 생성하고 Pass별 파라미터, Model별 파라미터,Material별 파라미터를 분리하여 바인딩 해주는 좋은 아이디어가 생각나지 않음


##프로젝트를 통해 성장한점


C++기초에 관해 많은 것들을 배울 수 있었다.


다양한 std클래스를 직접 사용해보고 자세히 알 수 있었고, 많은 문제들을 우회하는 방법들을 알게되었다.


메모리와 멀티스레드 환경에서 발생하는 동기화 문제에 깊게 생각하게 되었다.


자신 능력밖의 일에 대해 알아보고 해결할 수 있는 능력이 늘었다.


선 설계의 중요성에 대해 인지하게 되었다.


디버깅 툴의 중요성에 대해 알게 되었다.





