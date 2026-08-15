import type {
  ApiErrorResponse,
  LoginPayload,
  LoginResponse,
  RegisterPayload,
  RegisterResponse,
  VerifyOtpPayload,
  VerifyOtpResponse,
} from "@/types/auth";

const API_URL = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8081";

class ApiError extends Error {
  status: number;
  constructor(message: string, status: number) {
    super(message);
    this.status = status;
  }
}

async function request<TResponse>(
  path: string,
  options: RequestInit = {}
): Promise<TResponse> {
  const res = await fetch(`${API_URL}${path}`, {
    method: "POST",
    credentials: "include", // send/receive the httpOnly access_token cookie
    headers: {
      "Content-Type": "application/json",
      ...(options.headers || {}),
    },
    ...options,
  });

  const data = await res.json().catch(() => ({}));

  if (!res.ok) {
    const err = data as ApiErrorResponse;
    throw new ApiError(err.error || "Something went wrong", res.status);
  }

  return data as TResponse;
}

export const authApi = {
  register: (payload: RegisterPayload) =>
    request<RegisterResponse>("/auth/register", {
      body: JSON.stringify(payload),
    }),

  login: (payload: LoginPayload) =>
    request<LoginResponse>("/auth/login", {
      body: JSON.stringify(payload),
    }),

  verifyOtp: (payload: VerifyOtpPayload) =>
    request<VerifyOtpResponse>("/auth/verifyOtp", {
      body: JSON.stringify(payload),
    }),

  logout: () =>
    request<{ message: string }>("/auth/logout", {
      body: undefined,
    }),
};

export { ApiError };