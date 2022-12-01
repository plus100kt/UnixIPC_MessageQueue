#include <time.h>

struct clientData {
    int client_num; // 클라이언트 id
    int attr;       // 속성(사용여부, 부폼요청, 부품 보내기)
    struct timespec time; // 송신 시간
};

/* type = 1 : attr = 0 미사용, attr = 1 사용중
 * type = 2 : attr = 1 부품 요청
 * type = 3 : attr = 1 부품 보내기 */
struct message
{
    long msg_type;
    struct clientData data;
};