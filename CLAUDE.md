# CLAUDE.md

이 파일은 Claude Code가 이 저장소에서 작업할 때 참고하는 가이드입니다.

## 프로젝트 개요

`SampleOrderSystem`은 가상의 반도체 회사 "S-Semi"를 위한 **반도체 시료 생산주문관리 시스템**입니다.
콘솔 기반 애플리케이션으로, 담당자가 메뉴를 통해 시료 등록, 주문 접수/승인/거절, 재고·생산 모니터링,
생산라인 조회, 출고 처리를 수행합니다.

- 요구사항 전체 문서: [`docs/PRD.md`](docs/PRD.md)
- 기능별 상세 명세: [`docs/FEATURES/`](docs/FEATURES) 디렉터리
- 설계 아키텍처: [`docs/architecture.md`](docs/architecture.md)
- 과제 출처: `[CRA_AI] Day3_개인과제_반도체시료관리.pdf` (4~23페이지 = Chapter 01 개인과제 개요, Chapter 02 기능 명세)

이 저장소는 "[미션2] 프로젝트 개발" 산출물이며, 아래 PoC 저장소에서 검증한 구조/패턴을 **참고**하여 새로 작성합니다.

## 관련 PoC 저장소

미션1에서 개별 저장소로 검증된 PoC들은 **설계 참고 자료일 뿐**이다. 각 PoC는 C++20 콘솔 앱이며
Visual Studio `.slnx` 솔루션 구조를 따른다.

| PoC 항목 | 저장소 | 이 프로젝트에서의 역할 |
|---|---|---|
| MVC 스켈레톤 코드 | [ConsoleMVC-ghyun.lee-3088](https://github.com/GanghyunLee/ConsoleMVC-ghyun.lee-3088) | Model/View/Controller 패키지 구조 및 메뉴 루프 패턴 참고 |
| 데이터 영속성 처리 | [DataPersistence-ghyun.lee-3088](https://github.com/GanghyunLee/DataPersistence-ghyun.lee-3088) | JSON 파싱(`JsonParser`/`JsonValue`) 및 저장/로드(`SaveManager`) 패턴 참고 |
| 데이터 모니터링 Tool | [DataMonitor-ghyun.lee-3088](https://github.com/GanghyunLee/DataMonitor-ghyun.lee-3088) | 저장된 주문 데이터를 조회하는 `OrderRepository` 패턴 참고 |
| Dummy 데이터 생성 Tool | [DummyDataGenerator-ghyun.lee-3088](https://github.com/GanghyunLee/DummyDataGenerator-ghyun.lee-3088) | 테스트용 더미 주문 데이터 생성(`OrderGenerator`) 패턴 참고 |

> **PoC는 참조(reference)로만 사용하며, 이 프로젝트가 PoC 저장소에 의존성을 가져서는 안 된다.**
> - Git submodule/subtree, NuGet/vcpkg 패키지 참조, `.vcxproj`의 외부 프로젝트 참조(Project Reference),
>   상대경로 `#include`로 다른 저장소를 직접 가리키는 방식 등 **PoC 저장소를 빌드/실행 시점에 참조하는
>   모든 형태를 금지**한다.
> - 재사용할 코드(예: `JsonValue`/`JsonParser`)는 PoC의 코드를 참고하여 이 저장소 안에 **직접 새로 작성/이식**한다.
>   이식한 이후에는 이 저장소의 코드가 정본(source of truth)이며, PoC 쪽 변경을 추적/동기화할 필요가 없다.
> - PoC들의 `OrderData`/`DummyOrder`는 `status`를 단순 문자열(`"Received"` 등)로 표현하는 단순화된 예시일 뿐이다.
>   본 프로젝트에서는 `docs/PRD.md`에 정의된 5가지 상태(`RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASE`)를
>   정식 도메인 모델로 사용해야 하며, PoC의 필드명을 그대로 베끼지 않는다.

## 기술 스택

- 언어: C++20 (`stdcpp20`)
- 빌드: Visual Studio 2022 이상, MSBuild, 솔루션 파일 `SampleOrderSystem.slnx`
- 실행 형태: 콘솔 애플리케이션 (담당자가 메뉴 번호를 입력하는 방식)
- 데이터 저장: 파일 기반 영속성 (JSON) — `DataPersistence` PoC의 구조를 따름
- 아키텍처: MVC (Model / View / Controller 패키지 분리) — `ConsoleMVC` PoC의 구조를 따름

### 빌드 명령

```
msbuild SampleOrderSystem.slnx /p:Configuration=Debug /p:Platform=x64
msbuild SampleOrderSystem.slnx /p:Configuration=Release /p:Platform=x64
```

또는 Visual Studio에서 `SampleOrderSystem.slnx`를 열어 빌드합니다.

## 아키텍처 원칙

**작업을 시작하기 전에 항상 [`docs/architecture.md`](docs/architecture.md)를 먼저 참조한다.** 레이어 구조,
도메인 모델(`Sample`/`Order`/`ProductionJob` 등 필드), 상태 전이, 데이터 흐름, 디렉터리 구조, 영속성 방식 등
설계 세부사항의 단일 소스(source of truth)는 이 문서이며, 아래는 요약일 뿐이다.

- **Model**: `Sample`(시료), `Order`(주문), `ProductionJob`/`ProductionQueue`(생산라인/생산큐) 등 도메인 엔티티와 저장소(Repository)
- **View**: 콘솔 입출력만 담당 (메뉴 출력, 입력 파싱). 도메인 로직을 포함하지 않음
- **Controller**: View의 입력을 받아 Model의 상태 전이 규칙(주문 승인/거절, 생산 완료, 출고 등)을 조율
- 주문 상태 전이, 재고 계산, 생산 큐(FIFO) 스케줄링 등 핵심 규칙은 Model 계층에 두고 단위 테스트로 검증 가능하게 작성
- 화면 레이아웃(pixel-level UI)은 PDF의 예시일 뿐 자유롭게 구성 가능 — 다만 표시해야 할 정보/기능은 `docs/FEATURES/`의 명세를 따름

## 문서 관리 규칙

- 요구사항이 바뀌면 `docs/PRD.md`와 관련 `docs/FEATURES/*.md`를 함께 갱신
- 새로운 기능 상세는 `docs/FEATURES/` 아래 파일로 분리하여 관리 (한 파일 = 한 메뉴/기능 영역)
- **`docs/architecture.md`는 항상 최신 상태를 유지해야 한다.** 레이어 구조, 도메인 모델 필드, 상태 전이 규칙,
  저장 포맷(JSON 스키마), 디렉터리 구조 등 설계에 영향을 주는 변경을 코드에 반영할 때는 같은 작업(같은 커밋
  또는 바로 다음 커밋) 안에서 `docs/architecture.md`도 함께 수정한다. 설계와 문서가 어긋난 상태로 두지 않는다.
- 구현 계획, 설계 노트 등은 필요 시 `docs/superpowers/plans`, `docs/superpowers/specs` 하위에 축적 (PoC 저장소들과 동일한 관례).
  큰 구조 변경은 먼저 여기에 설계 노트를 남기고, 확정된 내용만 `docs/architecture.md`에 반영한다.

## 기능 구현 파이프라인 (서브에이전트 로스터)

`docs/FEATURES/`의 각 기능을 구현할 때는 아래 5개 프로젝트 전용 서브에이전트(`.claude/agents/`)를 이 순서로
사용한다. 설계 근거는 [`docs/superpowers/specs/2026-07-15-agent-architecture-design.md`](docs/superpowers/specs/2026-07-15-agent-architecture-design.md) 참고.

1. **`test-writer`** — 대상 FEATURES 파일 + PRD만 보고 실패하는(red) 테스트를 먼저 작성. 프로덕션 코드는 쓰지 않음.
2. **`implementer`** — 테스트를 건드리지 않고 통과(green)시키는 Model/View/Controller 코드 작성.
3. **`spec-verifier`**와 **`reviewer`** — 서로 독립적으로(서로의 결론을 참조하지 않고) 병렬 실행.
   `spec-verifier`는 테스트가 스펙을 실제로 증명하는지 회의적으로 재검증하고, `reviewer`는 클린코드/계층분리/
   PoC 무의존성 규칙 준수를 점검한다.
4. **`docs-sync`** — 위 두 검증을 통과한 뒤, `docs/architecture.md`가 실제 구현과 어긋났으면 최소 수정.

반려(2회까지 자동 반복 후 초과 시 사용자에게 보고)와 세부 규칙은 설계 스펙 3절을 따른다.

## 테스트 & 커밋

- 상태 전이(RESERVED→CONFIRMED/PRODUCING, PRODUCING→CONFIRMED, CONFIRMED→RELEASE, 거절→REJECTED), 재고 계산,
  실 생산량(`ceil(부족분 / 수율)`) 계산 등 핵심 로직은 단위 테스트로 검증
- 커밋은 작은 단위로 나누고, 무엇을 왜 바꿨는지 명확히 작성 (Commit 이력이 평가 항목에 포함됨)
- 클린 코드 원칙 준수: 불필요한 주석/추상화 지양, 명확한 이름, 계층 간 책임 분리 유지
