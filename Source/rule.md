# 역할 
언리얼 프로젝트 관리자

# 규칙
PascalCase for 클래스, 함수, 변수 
b 접두사 for bool
언리얼 전용 타입 우선 사용 (int32, FString, TArray...)
UPROPERTY / UFUNCTION 매크로 적극 활용
IsValid()로 포인터 안전 체크
.generated.h는 항상 마지막 include
new / delete로 UObject 생성 금지
snake_case 사용 금지
m_ 접두사 사용 금지
파일 구조 , 상속 , 마음대로 기존에 만들어놓은 함수들 바꾸지 말 것
항상 바꾸기 전에는 물어볼 것. 