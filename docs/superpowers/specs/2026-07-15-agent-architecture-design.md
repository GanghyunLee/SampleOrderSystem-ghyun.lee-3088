# 설계: SampleOrderSystem 전용 서브에이전트 아키텍처

- 날짜: 2026-07-15
- 배경: 실제 구현(C++ MVC 콘솔 앱) 단계를 앞두고, 확증편향(구현자가 자신의 구현을 자신이 검증하는 문제)을
  방지하고, 반복될 7개 기능(`docs/FEATURES/01~07`) 구현 절차를 일관되게 만들기 위해 프로젝트 전용
  커스텀 서브에이전트 구조를 도입한다.
- 관련 문서: [`../../PRD.md`](../../PRD.md), [`../../architecture.md`](../../architecture.md),
  [`../../../CLAUDE.md`](../../../CLAUDE.md)

## 1. 목표

- 테스트 작성과 구현을 서로 다른 인격(에이전트)이 담당하게 하여, "구현에 맞춰 테스트를 쓰는" 확증편향을 차단한다.
- 구현 완료 후에도 "테스트를 썼고 통과했다"를 그대로 신뢰하지 않고, 스펙 대비 커버리지를 별도로 재검증한다.
- 클린코드/아키텍처 규칙 준수 여부를 별도 리뷰어가 점검한다.
- `docs/architecture.md`가 실제 구현과 어긋나지 않도록 문서 동기화를 절차화한다.
- 과제 규정(Sonnet / Effort: Medium만 사용 가능, Opus 사용 이력 시 부정행위 처리)을 서브에이전트 설정에서도
  실수로 위반하지 않도록 한다.

## 2. 에이전트 로스터

| 서브에이전트 | 파일 | 역할 | 확증편향 방지 관점 |
|---|---|---|---|
| `test-writer` | `.claude/agents/test-writer.md` | 구현 착수 **전에** `docs/FEATURES/*.md`+`docs/PRD.md`만 근거로 실패하는(red) 단위 테스트 작성. 프로덕션 코드는 쓰지 않음 | 테스트가 "이미 짜인 구현"을 베끼지 않고 순수 스펙에서만 나옴 |
| `implementer` | `.claude/agents/implementer.md` | `test-writer`가 만든 테스트를 통과(green)시키는 Model/View/Controller 코드 작성. 테스트 파일은 절대 수정 불가 | 구현자가 테스트를 임의로 고쳐 통과시키는 것을 원천 차단 |
| `spec-verifier` | `.claude/agents/spec-verifier.md` | 구현+테스트 완료 후, 스펙을 처음부터 다시 읽고 "테스트가 실제로 요구사항을 검증하는지/누락된 케이스는 없는지"를 회의적으로 점검. 코드/테스트 작성 이력이 없는 독립된 시선 | 이 설계의 핵심 목적 — "테스트 통과"≠"요구사항 충족"을 별도 인격이 재검증 |
| `reviewer` | `.claude/agents/reviewer.md` | C++ 클린코드, MVC 계층 분리, PoC 무의존성 규칙(`CLAUDE.md`) 준수 여부 리뷰 | 구현자가 자기 코드의 품질 문제를 못 보는 것 방지 |
| `docs-sync` | `.claude/agents/docs-sync.md` | 구현 후 `docs/architecture.md`가 실제 코드(필드명/상태전이/디렉터리 구조)와 어긋났는지 확인·최소 갱신 | 문서 드리프트 방지 (`CLAUDE.md`에 명시된 의무의 절차화) |

공통 원칙:

- 5개 모두 frontmatter에 `model` 필드를 넣지 않는다 (세션의 Sonnet/Medium을 그대로 상속). 과제 규정상 다른
  모델 사용 이력이 남으면 부정행위로 간주되므로, 서브에이전트가 실수로 다른 모델을 쓰게 만들 여지를 없앤다.
- `test-writer`/`implementer` 간의 "서로의 산출물을 되돌릴 수 없다"는 제약은 tool 권한이 아니라 각 에이전트
  프롬프트에 명시된 규칙 + `reviewer`의 사후 감시로 강제한다 (Claude Code가 파일 단위 쓰기 권한을 도구
  레벨에서 세분화하지 못하기 때문).

## 3. 파이프라인 (기능 1개 구현 시 실행 순서)

```
[사용자] "FEATURES/02-sample-management.md 구현해줘" (feature/sample-management 브랜치)
    │
    ▼
① test-writer   ─ FEATURES/02 + PRD만 읽고 실패하는 단위 테스트 작성 (red)
    │              커밋: "Add failing tests for sample management (TDD)"
    ▼
② implementer   ─ 테스트를 건드리지 않고 Model/View/Controller 구현, 테스트 통과 (green)
    │              커밋: "Implement sample management to pass tests"
    ▼
③ spec-verifier ─┐  (병렬, 둘 다 read-only, 서로의 결론을 보지 않음)
   reviewer      ─┘
    │
    ├─ 문제 없음 → ④로 진행
    └─ 문제 있음 → 원인에 따라 되돌림
         - "테스트 자체가 스펙을 잘못 해석함" → ① test-writer 재작업
         - "구현이 스펙/클린코드 기준 미달" → ② implementer 재작업
         - 반려는 최대 2회까지만 자동 반복, 그 이상이면 메인 세션이 개입해 사용자에게 보고 (무한 루프 방지)
    ▼
④ docs-sync     ─ docs/architecture.md가 실제 구현과 어긋났으면 갱신
    │
    ▼
[메인 세션] 최종 커밋 정리 + 필요 시 PR
```

- 호출 방식: Workflow(다중 에이전트 자동 오케스트레이션 스크립트)는 사용하지 않는다. 메인 세션(Claude Code
  대화)이 파이프라인 순서대로 `Agent` 도구의 `subagent_type`에 각 이름을 지정해 수동으로 호출한다. 소규모
  콘솔 앱 하나를 다루는 이 프로젝트 규모에서는 자동 오케스트레이션보다 메인 세션이 각 단계 결과를 직접
  확인하며 제어하는 편이 적절하다.

## 4. 서브에이전트 파일 상세

저장 위치: 저장소 루트 `.claude/agents/` (git에 커밋되어 팀/평가자 모두 동일하게 재현 가능).

| 파일 | `tools` | 핵심 프롬프트 제약 |
|---|---|---|
| `test-writer.md` | `Read, Write, Glob, Grep, Bash` | `docs/FEATURES/*.md` + `docs/PRD.md`만 근거로 사용. 테스트 파일(`*Test.cpp` 또는 `Tests/` 하위)만 생성/수정. Model/View/Controller 프로덕션 파일은 절대 쓰지 않음. 작성 후 반드시 빌드하여 "실패(red)"함을 확인하고 종료 |
| `implementer.md` | `Read, Write, Edit, Glob, Grep, Bash` | `docs/architecture.md` 구조를 따름. 테스트 파일은 절대 수정 불가. 빌드+테스트가 green이 될 때까지 반복 |
| `spec-verifier.md` | `Read, Grep, Glob, Bash` (Write/Edit 없음) | 코드/테스트 작성 이력이 없는 것처럼 행동. 해당 `docs/FEATURES/*.md`의 요구사항을 항목별 체크리스트로 쪼개 "테스트로 검증되는가/누락되었는가"를 판정. 기본 태도는 회의적 — 설명이 그럴듯해도 테스트로 증명되지 않으면 미검증으로 표기 |
| `reviewer.md` | `Read, Grep, Glob, Bash` (Write/Edit 없음) | `CLAUDE.md`의 클린코드/계층분리/PoC 무의존성 규칙을 체크리스트로 사용. 발견사항만 보고, 직접 수정하지 않음 |
| `docs-sync.md` | `Read, Edit, Glob, Grep, Bash` | 코드(필드명/상태전이/디렉터리 구조)와 `docs/architecture.md`를 비교, 어긋난 부분만 최소 수정. 요구사항 자체(PRD/FEATURES)는 건드리지 않음 |

## 5. 테스트 전략 (이 설계 자체에 대한 검증)

이 설계는 코드가 아니라 프로세스/설정이므로 "테스트"는 다음으로 대체한다.

- 5개 `.claude/agents/*.md` 파일이 실제로 생성되고 frontmatter가 유효한지 확인.
- 파일럿으로 `docs/FEATURES/02-sample-management.md` 하나를 이 파이프라인으로 시험 실행해, 반려 로직과
  역할 분리가 실제로 의도대로 동작하는지 확인 (구현 단계 진입 시 첫 실사용으로 검증).

## 6. 범위 밖 (Out of scope)

- Workflow 기반 완전 자동 오케스트레이션 (사용자가 명시적으로 "ultracode"/워크플로 사용을 요청하는 경우
  별도로 재검토).
- 5개 외 추가 역할(예: 성능 최적화 전담 에이전트) 도입은 실제 구현 중 필요성이 확인되면 이 문서를 갱신하여
  추가한다.
