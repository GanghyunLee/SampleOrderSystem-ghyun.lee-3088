# 설계: 구현 로드맵 (전체 작업 순서)

- 날짜: 2026-07-15
- 배경: `docs/PRD.md`, `docs/FEATURES/*.md`, `docs/architecture.md`, 5개 서브에이전트
  파이프라인(`.claude/agents/`) 설계는 완료되었으나 실제 소스 코드는 아직 없다(`.vcxproj` 스켈레톤만 존재).
  그린필드 구현에 들어가기 전, 어떤 순서로 무엇을 만들지 확정한다.
- 관련 문서: [`../../PRD.md`](../../PRD.md), [`../../architecture.md`](../../architecture.md),
  [`../../../CLAUDE.md`](../../../CLAUDE.md),
  [`2026-07-15-agent-architecture-design.md`](2026-07-15-agent-architecture-design.md)

## 1. 이 문서의 목적

구현해야 할 8개 단위(기반 공사 1개 + FEATURES 7개)는 서로 의존관계가 있어 순서를 정해야 한다.
이 문서는 그 순서와 각 단위의 착수 조건만 정의한다. 각 단위의 세부 설계(테스트 케이스, 클래스 구조 등)는
해당 단위를 실제로 착수할 때 별도 브레인스토밍/계획 문서로 만든다.

## 2. 단위 목록과 순서

| 순서 | 단위 | 대응 문서 | 착수 조건(의존성) |
|---|---|---|---|
| 0 | 기반 공사 (프로젝트 스켈레톤 + JSON 영속성 이식) | 특정 FEATURES 없음. `architecture.md` §2, §6, §7 근거 | 없음 (최초 착수) |
| 1 | 시료 관리 | `FEATURES/02-sample-management.md` | 0단계의 `Sample`/`SampleRepository`/JSON 저장 골격 |
| 2 | 시료 주문(접수) | `FEATURES/03-order-registration.md` | 1단계의 등록된 Sample 존재 |
| 3 | 주문 승인/거절 | `FEATURES/04-order-approval-rejection.md` | 2단계의 `RESERVED` 주문 + `ProductionQueue`/`ProductionJob` 신규 도입 |
| 4 | 생산 라인 | `FEATURES/06-production-line.md` | 3단계에서 등록된 생산 큐 |
| 5 | 출고 처리 | `FEATURES/07-shipping.md` | 3~4단계에서 만들어지는 `CONFIRMED` 주문 |
| 6 | 모니터링 | `FEATURES/05-monitoring.md` | 1~5단계 데이터(주문 상태별 분포, 재고)가 실제로 존재해야 검증 의미가 있음 |
| 7 | 메인 메뉴 통합 | `FEATURES/01-main-menu.md` | 1~6단계의 모든 도메인 Controller 존재 |

### 순서 결정 근거

- 도메인 의존성이 사슬처럼 이어진다: Sample 없이 Order 불가, RESERVED 주문 없이 승인/거절 불가,
  승인/거절에서 생기는 생산 큐 없이 생산 라인 조회 불가, CONFIRMED 주문 없이 출고 불가.
- 모니터링(6번)은 로직 자체는 이르게 만들 수도 있지만, "여유/부족/고갈", "상태별 주문 수" 같은 항목을
  의미 있게 검증하려면 여러 상태의 실제 데이터가 있는 편이 스펙 검증에 유리해 뒤쪽에 배치한다.
- 메인 메뉴(7번)는 `AppController`가 각 도메인 Controller를 호출/라우팅하는 조립 단계이므로,
  조립 대상이 모두 존재한 뒤에 진행하는 것이 자연스럽다.

## 3. 각 단위의 실행 방식

- 모든 단위(0단계 포함)는 동일한 5단계 파이프라인을 적용한다:
  `test-writer → implementer → (spec-verifier ∥ reviewer) → docs-sync`
  (상세는 [`2026-07-15-agent-architecture-design.md`](2026-07-15-agent-architecture-design.md) 참고)
- 0단계는 대응하는 FEATURES 문서가 없으므로, `test-writer`/`implementer`에게 근거로 줄 스펙은
  `docs/architecture.md` §2(레이어 구조)·§6(데이터 영속성)·§9(테스트 전략의 JSON round-trip 요구)로 대체한다.
- [[feedback_branch_strategy]]에 따라 각 단위는 `feature/<단위명>` 브랜치에서 작업한다
  (예: `feature/foundation`, `feature/sample-management`, `feature/order-registration`, ...).
- 반려(spec-verifier/reviewer가 문제 제기) 시 처리 규칙은 에이전트 아키텍처 설계 문서 §3을 그대로 따른다
  (원인별로 test-writer 또는 implementer로 되돌리고, 2회 초과 반려 시 사용자에게 보고).

## 4. 진행 방식

- 이 문서는 전체 로드맵(순서)에 대한 합의만 기록한다.
- 실제 각 단위(0~7)의 착수는 이 대화가 아니라 사용자가 그때그때 "0단계 시작해줘" 등으로 별도 요청할 때
  이루어진다. 착수 시점에 해당 단위에 대한 상세 설계(필요한 경우) 또는 바로 파이프라인 실행으로 들어간다.

## 5. 범위 밖

- 각 단위 내부의 세부 클래스/함수 설계는 이 문서에서 다루지 않는다 (착수 시점에 별도 문서화).
- 8개 단위 순서 변경이 필요해지면(예: 구현 중 의존성 재발견) 이 문서를 갱신한다.
