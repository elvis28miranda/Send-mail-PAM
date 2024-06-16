#include <stdio.h>
#include <stdlib.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <curl/curl.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

int codigo[7];
char codigo_string[7];
int codigo_int;
int digito_aleatorio;
char toAddr[100];

void recipient(const char *username) {
    struct {
        const char *name;
        const char *email;
    } users[] = {
        {"arthur", "eita@gmail.com"},
        {"elvis", "teste@gmail.com"},
        {"joao", "jpm83633@gmail.com"},
	{"john", "jpm83633@gmail.com"},
        {"rafael", "aaa@gmail.com"}
    };

    int found = 0;
    for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++) {
        if (strcmp(username, users[i].name) == 0) {
            strcpy(toAddr, users[i].email);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("\nNome não encontrado\n");
        // Handle the case where the username is not found
    }
}

void generateCode() {
    srand(time(NULL));

    for (int i = 0; i < 6; i++) {
        digito_aleatorio = rand() % 10;
        codigo[i] = digito_aleatorio;
    }

    for (int i = 0; i < 6; i++) {
        codigo_string[i] = codigo[i] + '0';
    }
    codigo_string[6] = '\0';

    codigo_int = atoi(codigo_string);
}

void sendMail() {
    generateCode();
    char payload_text[250];

    #define FROM_ADDR    "autenticacao2Factor@outlook.com"
    #define TO_ADDR      toAddr
    #define CC_ADDR      "joao.meneguesso@fatec.sp.gov.br"

    snprintf(payload_text, sizeof(payload_text),
             "Date: Thu, 29 Mar 2024 19:51:00 +1100\r\n"
             "To: %s\r\n"
             "From: %s\r\n"
             "Cc: %s\r\n"
             "Subject: Autenticacao 2 fatores\r\n"
             "\r\n"
             "Codigo de autenticacao: %s \r\n"
             "\r\n",
             TO_ADDR, FROM_ADDR, CC_ADDR, codigo_string);

    struct upload_status {
        size_t bytes_read;
    };

    size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp) {
        struct upload_status *upload_ctx = (struct upload_status *)userp;
        const char *data;
        size_t room = size * nmemb;

        if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
            return 0;
        }

        data = &payload_text[upload_ctx->bytes_read];

        if (data) {
            size_t len = strlen(data);
            if (room < len) {
                len = room;
            }
            memcpy(ptr, data, len);
            upload_ctx->bytes_read += len;

            return len;
        }

        return 0;
    }

    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct upload_status upload_ctx = { 0 };

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, "autenticacao2Factor@outlook.com");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "autenticacao2PAM");
        curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp-mail.outlook.com:587");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_ADDR);

        recipients = curl_slist_append(recipients, TO_ADDR);
        recipients = curl_slist_append(recipients, CC_ADDR);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *username;
    int retval = pam_get_user(pamh, &username, NULL);

    if (retval != PAM_SUCCESS) {
        return retval;
    }

    recipient(username);

    printf("\nAguarde alguns instantes para a chegada do codigo no email!\n");
    sendMail();

    /*char resposta[7];
    printf("\nInforme o codigo: ");
    fgets(resposta, sizeof(resposta), stdin); */

    char *resposta = NULL;
    retval = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resposta, "\nInforme o codigo: ");
    if(retval != PAM_SUCCESS) {
	return retval;
    }

    if (strcmp(resposta, codigo_string) == 0) {
        printf("\nValidacao concluida, autenticacao liberada\n");
        return PAM_SUCCESS;
    } else {
        printf("\nCodigo diferente, autenticacao negada\n");
        return PAM_AUTH_ERR;
    }
}
