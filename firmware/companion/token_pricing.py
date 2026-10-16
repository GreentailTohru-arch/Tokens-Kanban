"""Standard API equivalent estimates, not Codex subscription charges.

Verified 2026-10-16 against https://developers.openai.com/api/docs/pricing
and each exact model's /api/docs/models/<model> page. No network lookup at runtime.
Rates are integer nano-USD per token (USD per million tokens multiplied by 1000).
All retained history is valued at this price snapshot; see docs/codex-token-display.md.
"""
PRICE_DATE = '2026-10-16'
# Ordinary input, cache read, cache write, output. Never guess unknown model aliases.
RATES = {
    'gpt-6.1-sol': (2000, 100, 2500, 10000),
    'gpt-6-sol': (2000, 200, 2500, 10000),
    'gpt-6-astra': (10000, 1000, 12500, 50000),
    'gpt-5.6-sol': (4000, 400, 5000, 20000),
    'gpt-5.6-luna': (200, 20, 250, 1200),
}


def request_nano_usd(model, usage):
    """Return None when model/counters are unknown; cache subsets never double count."""
    rates = RATES.get(model) if isinstance(model, str) else None
    if rates is None:
        return None
    values = [usage.get(k) for k in ('input_tokens', 'cached_input_tokens',
                                    'cache_write_input_tokens', 'output_tokens')]
    if any(type(v) is not int or not 0 <= v <= 10**15 for v in values):
        return None
    incoming, cached, written, output = values
    if cached + written > incoming:
        return None
    normal, read_rate, write_rate, output_rate = rates
    if incoming > 272000:
        normal *= 2; read_rate *= 2; write_rate *= 2; output_rate = output_rate * 3 // 2
    return ((incoming - cached - written) * normal + cached * read_rate
            + written * write_rate + output * output_rate)
