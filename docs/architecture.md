# 아키텍처 설계

이 문서는 `SampleOrderSystem`의 설계 아키텍처(레이어 구조, 도메인 모델, 데이터 흐름, 저장소 구조)를 정리한다.
요구사항 자체는 [`docs/PRD.md`](PRD.md)와 [`docs/FEATURES/`](FEATURES)를 따르며, 이 문서는 "무엇을 만들지"가 아니라
"어떻게 구성할지"에 대한 설계 기준이다.

> **이 문서는 살아있는 문서(Living Document)다.** 구조/모델/저장 방식에 변경이 생기면 코드 변경과
> 같은 커밋(또는 바로 다음 커밋)에서 이 문서를 함께 갱신한다. 규칙은 [`CLAUDE.md`](../CLAUDE.md) 참고.

## 1. 기술 스택

| 구분 | 내용 |
|---|---|
| 언어 | C++20 (`stdcpp20`) |
| 빌드 도구 | Visual Studio 2022 이상, MSBuild |
| 솔루션 형식 | `SampleOrderSystem.slnx` |
| 실행 형태 | 콘솔 애플리케이션 (메뉴 번호 입력 방식) |
| 데이터 저장 | 파일 기반 JSON 영속성 (`DataPersistence` PoC 구조 이식, §6 참고) |
| 아키텍처 패턴 | MVC (Model/View/Controller 계층 분리, `ConsoleMVC` PoC 구조 참고) |

```
msbuild SampleOrderSystem.slnx /p:Configuration=Debug /p:Platform=x64
msbuild SampleOrderSystem.slnx /p:Configuration=Release /p:Platform=x64
```

## 2. 레이어 구조 (MVC)

`ConsoleMVC` PoC의 구조를 계승하여 3계층으로 분리한다.

```
main.cpp
  └─ AppController            // 메인 메뉴 라우팅, 각 도메인 Controller 보유/호출
       ├─ SampleController    // 시료 등록/조회/검색
       ├─ OrderController     // 주문 접수, 승인/거절
       ├─ MonitoringController// 주문량/재고량 모니터링
       ├─ ProductionController// 생산 라인 현황/큐 조회, 생산 진행/완료 처리
       └─ ShippingController  // 출고 처리

View (콘솔 입출력 전용, 도메인 로직 없음)
  ├─ MainMenuView
  ├─ SampleView
  ├─ OrderView
  ├─ MonitoringView
  ├─ ProductionView
  └─ ShippingView

Model (도메인 엔티티 + 상태 전이/계산 규칙 + Repository)
  ├─ Sample, Order, ProductionJob (엔티티)
  ├─ OrderStatus (열거형 상태)
  ├─ SampleRepository, OrderRepository (저장/조회)
  └─ ProductionQueue (FIFO 생산 큐)
```

### 계층별 책임

| 계층 | 책임 | 하지 않는 것 |
|---|---|---|
| View | 메뉴 출력, 사용자 입력 수집/파싱, 결과 포맷팅 출력 | 상태 전이 판단, 재고 계산, 파일 I/O |
| Controller | View의 입력을 받아 Model 연산 호출, 여러 Model/Repository 간 흐름 조율 | 콘솔 입출력 직접 수행, 저장 포맷(JSON) 세부 처리 |
| Model | 엔티티, 상태 전이 규칙, 재고/생산량 계산, 영속성 저장/로드 | 콘솔 출력 문자열 생성 |

- View는 Controller가 반환한 값(성공/실패, 조회 결과 DTO 등)을 그대로 출력만 한다.
- 에러는 예외(exception)가 아니라 **성공/실패를 나타내는 반환값(bool 또는 Result 타입)** 으로 표현한다
  (`ConsoleMVC` PoC의 관례를 계승). View는 실패 시 안내 메시지를 출력하고 재입력을 유도한다.

## 3. 도메인 모델

### 3.1 Sample (시료)

```cpp
struct Sample {
    std::string id;              // 예: "S-001"
    std::string name;
    double avgProductionTimeMin; // 평균 생산시간 (분/개)
    double yield;                // 수율 (0 < yield <= 1)
    int stock = 0;               // 현재 재고
};
```

### 3.2 Order (주문)

```cpp
enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE };

struct Order {
    std::string orderId;     // 예: "ORD-20260416-0043"
    std::string sampleId;    // Sample.id 참조
    std::string customerName;
    int quantity = 0;
    OrderStatus status = OrderStatus::RESERVED;
    // 선택: createdAt, approvedAt, releasedAt 등 타임스탬프
};
```

### 3.3 ProductionJob (생산 큐 항목)

```cpp
struct ProductionJob {
    std::string orderId;     // 대상 주문
    std::string sampleId;
    int shortage = 0;        // 부족분 = 주문 수량 - 승인 시점 재고
    int actualQuantity = 0;  // 실 생산량 = ceil(shortage / yield)
    double totalTimeMin = 0; // 총 생산 시간 = avgProductionTimeMin * actualQuantity
    double progress = 0;     // 0.0 ~ 1.0 (콘솔 조회 시 표시용)
};
```

- `ProductionQueue`는 `ProductionJob`을 FIFO로 보관하는 큐(`std::deque` 등)로 구현한다.
- 생산 완료 처리 시 해당 `Order.status`를 `PRODUCING → CONFIRMED`로 변경하고, 초과 생산분(수율 보정으로 인한
  잉여)은 `Sample.stock`에 더한다.

## 4. 상태 전이 (Order.status)

```
RESERVED --(승인, 재고 충분)--> CONFIRMED
RESERVED --(승인, 재고 부족)--> PRODUCING   (ProductionQueue에 ProductionJob 등록)
RESERVED --(거절)-------------> REJECTED
PRODUCING --(생산 완료)--------> CONFIRMED
CONFIRMED --(출고 처리)--------> RELEASE
```

상태 전이는 반드시 `Order` 또는 관련 서비스(Model 계층) 안에서 검증한다. 예를 들어 `RESERVED`가 아닌 주문에
대한 승인/거절 시도, `CONFIRMED`가 아닌 주문에 대한 출고 시도는 Model 계층에서 거부하고 Controller에 실패를
반환한다. 자세한 규칙은 [`docs/FEATURES/04-order-approval-rejection.md`](FEATURES/04-order-approval-rejection.md),
[`docs/FEATURES/07-shipping.md`](FEATURES/07-shipping.md) 참고.

## 5. 데이터 흐름 예시 (주문 승인 → 생산 → 출고)

```
[OrderController.Approve(orderId)]
    → OrderRepository.find(orderId)                      // RESERVED 검증
    → SampleRepository.find(order.sampleId)               // 재고 조회
    → 재고 >= 주문 수량 ?
         Yes → order.status = CONFIRMED
               SampleRepository.decreaseStock(sampleId, quantity)
         No  → shortage = quantity - stock
               job = ProductionJob{ shortage, actualQuantity = ceil(shortage/yield), ... }
               ProductionQueue.enqueue(job)
               order.status = PRODUCING
    → OrderRepository.save(order)  // 영속화

[ProductionController.Tick() / CompleteCurrentJob()]
    → ProductionQueue.front() 처리 완료 시
    → SampleRepository.addStock(job.sampleId, job.actualQuantity)
    → SampleRepository.decreaseStock(job.sampleId, job.shortage 해당 주문 소비분)
    → OrderRepository.find(job.orderId).status = CONFIRMED
    → ProductionQueue.dequeue()

[ShippingController.Release(orderId)]
    → OrderRepository.find(orderId)  // CONFIRMED 검증
    → order.status = RELEASE
    → OrderRepository.save(order)
```

## 6. 데이터 영속성

`DataPersistence` PoC의 구조(`JsonValue`, `JsonParser`, `SaveManager`)를 재사용/확장한다.

- 저장 형식: JSON 파일 (예: `data/samples.json`, `data/orders.json`, 또는 단일 `data/store.json`에 통합)
- 저장 시점: 각 상태 변경 트랜잭션(등록/승인/거절/생산완료/출고) 직후 즉시 저장 (in-memory 캐시 + write-through)
- 로드 시점: 애플리케이션 시작 시 1회 전체 로드
- Repository 인터페이스는 저장 포맷을 캡슐화하여, Controller/Model 로직이 JSON 구조를 직접 알 필요가 없도록 한다.
- `DummyDataGenerator` PoC로 생성한 더미 주문 데이터도 동일 스키마(JSON)로 적재 가능해야 한다.
- `DataMonitor` PoC는 이 JSON 저장소를 별도 프로세스에서 읽기 전용으로 조회하는 도구로,
  저장 포맷을 변경할 때는 `DataMonitor`/`DummyDataGenerator`와의 스키마 호환성도 함께 검토한다.

## 7. 디렉터리 구조 (제안)

```
SampleOrderSystem/
  SampleOrderSystem.slnx
  SampleOrderSystem/
    main.cpp
    Model/
      Sample.h / .cpp
      Order.h / .cpp
      OrderStatus.h
      ProductionJob.h / .cpp
      ProductionQueue.h / .cpp
      SampleRepository.h / .cpp
      OrderRepository.h / .cpp
      JsonValue.h / .cpp        // DataPersistence PoC에서 이식
      JsonParser.h / .cpp       // DataPersistence PoC에서 이식
      SaveManager.h / .cpp      // DataPersistence PoC에서 이식
    View/
      MainMenuView.h / .cpp
      SampleView.h / .cpp
      OrderView.h / .cpp
      MonitoringView.h / .cpp
      ProductionView.h / .cpp
      ShippingView.h / .cpp
    Controller/
      AppController.h / .cpp
      SampleController.h / .cpp
      OrderController.h / .cpp
      MonitoringController.h / .cpp
      ProductionController.h / .cpp
      ShippingController.h / .cpp
  data/
    store.json (또는 samples.json / orders.json)
```

> 위 구조는 제안이며, 실제 구현 시 파일 분할 수준은 조정 가능하다. 다만 Model/View/Controller 폴더 분리
> 자체는 유지한다.

## 8. 동시성/실행 모델

- 단일 스레드, 동기 콘솔 애플리케이션. 생산 진행은 실시간 타이머가 아니라 사용자가 "생산 라인 조회"
  또는 "틱 진행" 등의 명령을 실행할 때 계산/갱신하는 방식으로 단순화한다 (실제 초단위 타이머 불필요).
- 생산 라인은 단일 라인(순차 처리)만 지원한다. 다중 라인 확장은 범위 밖([`docs/PRD.md`](PRD.md) §9 참고).

## 9. 테스트 전략

핵심 로직은 Model 계층에 격리되어 있으므로 콘솔 입출력 없이 단위 테스트 가능해야 한다. 최소 커버 대상:

- 주문 상태 전이 규칙 (허용/거부 케이스)
- 재고 충분/부족 분기 및 부족분 계산
- 실 생산량(`ceil(shortage / yield)`), 총 생산 시간 계산
- 생산 큐 FIFO 순서 보장
- 모니터링 재고 상태 분류(여유/부족/고갈) 임계값 로직
- JSON 저장/로드 왕복(round-trip) 시 데이터 무손실

## 10. 변경 이력 관리

- 도메인 모델 필드 추가/변경, 상태 전이 규칙 변경, 저장 포맷 변경, 디렉터리 구조 변경 시 이 문서를 갱신한다.
- 큰 구조 변경은 `docs/superpowers/plans` 또는 `docs/superpowers/specs`에 설계 노트를 남긴 뒤, 확정된 내용만
  이 문서에 반영한다.
