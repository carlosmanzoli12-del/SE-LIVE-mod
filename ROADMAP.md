# StreamForge roadmap

## Foundation (v0.1.0)
- [x] OBS dock lifecycle and menu registration
- [x] Stream/record controls and replay save
- [x] Scene list and scene switching
- [x] Source visibility controls
- [x] Browser overlay source creation
- [x] GitHub Actions build workflow targeting OBS 30.0.2

## Next implementation milestones
- [ ] Persist destination profiles using OBS config storage with OS credential vault integration for secrets
- [ ] Multiple RTMP outputs with independent service/encoder/output instances, start/stop, reconnect and per-destination health
- [ ] Twitch and YouTube OAuth sign-in and API adapters
- [ ] TikTok Live integration only where the account is eligible and an authorized RTMP URL/key is supplied by TikTok
- [ ] Unified chat message model, provider adapters, moderation actions and rate-limit handling
- [ ] Activity feed adapters for follows, subscriptions, memberships, gifts and platform events
- [ ] Overlay/alert library, browser-source management, preview and per-scene placement
- [ ] Localization, accessibility, crash-safe settings migration and automated integration tests

## Security principles
- Never commit stream keys, OAuth client secrets, refresh tokens or personal data.
- Use OAuth authorization-code flows with state/PKCE where supported.
- Store refresh tokens in the operating system credential store, not plain-text settings.
- Treat all chat messages and overlay content as untrusted input.
- Do not bypass platform access controls or eligibility requirements.
