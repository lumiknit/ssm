# Note

## Concept

- 실행하는 순서는 (1) VM 생성, (2) bytecode 불러옴, (3) VM에 bytecode append
(4) append 하면 global, code 영역에 특정 값이 추가됨,
(5) run 에 entrypoint 넣어주면 그 지점부터 실행함.
- GC, global, code 영역은 VM struct 내에 저장
- VM을 통째로 삭제하지 않는 한, 개별적으로 global/code 제거는 불가능
- VM 내에 global table이 있어서, global index -> real pointer 매핑이 있음.
- code list도 저장해둠.
- bytecode `.ssm`
- bytecode에는 global 영역, code 영역이 있음.
- 상수는 global 영역에 그대로 저장함.
- 상수가 아닌 global은 개수만 기록함.
- code에서 global로 사용되는 파라메터에는 정수가 들어가는데,
- bytecode loader 에서 이걸 pointer로 바꿔줘야 함.
- bytecode는 ocaml의 opcode랑 비슷하게 동작
- stack은 고정 사이즈 할당을 하는데,
- 함수 호출 등 원래 비용 큰 시점에서 stack에 여유 공간이 일정량 이상 되도록
- 할당해줌.
- 실행할 때는 타입체킹 없음
- 문자열 같이 tagged tuple이 아닌 bytes 는 large object로 할당.
- large object는 헤더만 있고, 거의 풀사이즈를 표현 간으.
- 실행시간에 타입체킹 없음.
- 기본적인 정수/실수는 bytes로 저장.
- tagged tuple이든 뭐든 모든 object는 4byte aligned는 하도록.
- LSB=0 인것만 GC 되는 object 취급.

## TODO

- `.ssm` 파일을 인자로 입력받음.
- GC 만들기
- Instruction은 OCaml VM 처럼
- Object도 int/float를 OCaml 방식 인코딩 시도
