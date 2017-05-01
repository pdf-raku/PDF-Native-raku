#ifndef __PDF_ENC_BASE64_H
#define __PDF_ENC_BASE64_H

#ifdef _WIN32p
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT extern
#endif

DLLEXPORT void pdf_encode_base64 (uint8_t* in, size_t inlen,
				  uint8_t* out, size_t outlen);

#endif /* __PDF_ENC_BASE64_H */
