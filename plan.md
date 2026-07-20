# Plan de adaptación Angular + .NET

## Objetivo

Mejorar `codebase-memory-mcp` para que represente correctamente proyectos Angular y
ASP.NET Core, usando `ppi-ntv` únicamente como validación privada. El fork público
debe contener solo lógica genérica y fixtures sintéticos.

Repositorio:
`/Users/MoreFer/dev/fielmann-ag/codebase-memory-mcp-angular-dotnet`

## Estado actual

- [x] Clonado el fork `fernandofielmann/codebase-memory-mcp-angular-dotnet`.
- [x] Compilado `main` y medida la línea base sobre `ppi-ntv`.
- [x] Confirmada la línea base: 13 rutas, 2 `HTTP_CALLS` y ninguna ruta MVC ASP.NET.
- [x] Creada y publicada la PR 1 para rutas ASP.NET Core.
- [x] Creada y publicada la PR 2 para llamadas Angular `HttpClient`.
- [x] Creada y publicada la PR 3 para wrappers HTTP y enlace completo
  frontend-backend.
- [x] Implementada y validada localmente la PR 4 para representar assets sin
  mezclarlos con rutas HTTP.
- [x] Integradas las cuatro PRs y validados 20/20 endpoints.
- [x] Instalado el binario validado y reconstruido el índice.
- [x] Diseñado el roadmap posterior de semántica Angular y .NET.
- [x] Implementada, validada y fusionada la PR 5 para metadatos Angular y
  dependencias standalone.
- [x] Implementada, validada y fusionada la PR prioritaria A para excluir
  código minificado en todos los modos.
- [x] Implementada, validada y fusionada la PR prioritaria B para completar
  rutas, entry points y handlers en `get_architecture`.
- [x] Implementada, validada y fusionada la PR prioritaria C para clasificar
  capas con evidencia y `confidence`.
- [x] Resuelta la calidad y completitud de `get_architecture` (PRs A, B y C).
- [x] Implementada, validada y fusionada la PR 6 para Angular Router y lazy
  loading (nodos `NavigationRoute` + aristas `ROUTES_TO`/`REDIRECTS_TO`/
  `LAZY_LOADS`/`DECLARES_ROUTE`); registro de los tipos nuevos en esquema,
  CLI y README completado en el follow-up PR 27.
- [x] Cerrados los issues asociados a PRs ya fusionados: #11 (PR 6),
  #9/#10/#7 (PRs A/B/C).
- [x] Implementada, validada localmente y fusionada la PR 11 para invocaciones
  genéricas C# (`AddScoped<TService,TImpl>`, `DoThing<int,string>`); PR #29
  fusionada en `main` (squash `3749ae9`).
- [ ] Continuar después el roadmap en PRs pequeñas y verificables.

El fork tiene habilitados los issues y no ejecuta checks automáticos en las PRs.
Las PR 1, PR 2, PR 3, PR 4 y PR 5 están fusionadas en `main`.
La PR prioritaria A está fusionada en `main` como PR 6.
La PR prioritaria B está fusionada en `main` como PR 8.
La PR prioritaria C está fusionada en `main` como PR 23 (squash, commit `0f1f3d1`).
La PR 6 (Angular Router) está fusionada en `main` como PR 25 (merge commit `ce1bd12`);
su follow-up de registro de esquema/CLI/README como PR 27 (merge commit `fbf6ab4`).
Issues cerrados: #7, #9, #10 y #11.
El bloque de calidad de `get_architecture` (A + B + C) queda cerrado.
La PR 11 (invocaciones genéricas C#) está implementada, validada localmente y
fusionada en `main` como PR #29 (squash commit `3749ae9`).

## Prioridad inmediata: calidad de `get_architecture`

Este bloque tiene prioridad sobre las PR 6–15 del roadmap. La validación privada
mostró que la arquitectura requiere demasiado contraste manual con `main.ts`,
`Program.cs` y los `.csproj`:

- Chart.js minificado contaminó los hotspots. El archivo concreto es
  `support/unfinished-customer-interactions/monitoring/scripts/WrongExitMonitoring/Reporting/Resources/chart.umd.min.js`;
  el `.csproj` lo declara como `EmbeddedResource`, no como código de aplicación.
- Varias capas quedaron mal clasificadas porque actualmente se infieren
  principalmente mediante fan-in/fan-out.
- Parte de las rutas y handlers no apareció en el resumen.

### PR prioritaria A: excluir código minificado en todos los modos

- [x] Mover `.min.js` y `.min.css` de `FAST_IGNORED_SUFFIXES` a
  `ALWAYS_IGNORED_SUFFIXES` en `src/discover/discover.c`.
- [x] Mantener `.cbmignore` como escape hatch por repositorio, no como solución
  necesaria para assets minificados comunes.
- [x] Añadir pruebas de descubrimiento para `full`, `moderate` y `fast`.
- [x] Verificar que `index_status` informe estos archivos como exclusiones
  deliberadas y no como fallos de cobertura.
- [x] Reindexar el proyecto privado desde cero y confirmar que Chart.js deja de
  aparecer en hotspots, packages, layers y clusters.

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/6  
Rama: `fix/ignore-minified-assets`  
Commit: `70d4e45`

Validación:

- Pruebas dirigidas `discover` e `index_resilience`: 97 pasaron.
- `make -j3 -f Makefile.cbm lint-ci`: correcto.
- `scripts/test.sh`: correcto.
- Índice privado reconstruido desde cero: 10.030 nodos y 27.069 aristas.
- Chart.js figura en `index_status` con `reason: ignored-suffix`, no tiene nodos
  en el grafo y no aparece en hotspots, packages, layers ni clusters.
- Línea base conservada: 20 callers Angular enlazados con 20 rutas ASP.NET y
  17 handlers.

Workaround privado hasta fusionar la corrección:

```gitignore
**/*.min.js
**/*.min.css
```

### PR prioritaria B: rutas y handlers completos

- [x] Eliminar el truncamiento silencioso causado por los `LIMIT 20` fijos de
  `arch_entry_points` y `arch_routes`.
- [x] Mantener un límite configurable o paginado y devolver explícitamente
  `total`, `shown` y `truncated`.
- [x] Ordenar los resultados de forma determinista antes de aplicar el límite.
- [x] Resolver handlers mediante las aristas `HANDLES`, usando
  `Route.properties.handler` solo como compatibilidad o fallback.
- [x] Añadir pruebas con más de 20 rutas, múltiples handlers, rutas sin handler y
  resultados truncados.
- [x] Validar que el resumen represente todas las rutas ASP.NET detectadas y los
  20/20 enlaces Angular→ASP.NET existentes.

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/8  
Rama: `fix/architecture-route-completeness`  
Commit: `005f28d`

Validación local:

- Suites dirigidas `store_arch` y `mcp`: 200 pruebas pasaron.
- `make -j3 -f Makefile.cbm lint-ci`: correcto.
- `scripts/test.sh`: correcto.
- Límite MCP configurable: 100 por defecto y 1.000 como máximo.
- Compatibilidad conservada con los arrays y el campo singular `handler`;
  `handlers` expone todos los destinos `HANDLES`.

Validación privada `ppi-ntv`:

- Índice `full`: 10.030 nodos y 27.069 aristas.
- `get_architecture` por defecto devuelve las 57 rutas y los 6 entry points,
  ambos con `truncated: false`.
- Las 41 aristas `HANDLES` aparecen como 41 rutas con handler; las 16 rutas sin
  handler son consumidores HTTP, rutas internas/de prueba o URLs externas.
- Se conservan 20 `DATA_FLOWS` Angular→ASP.NET; las 21 `HTTP_CALLS` incluyen
  además una llamada sin enlace backend.
- Con `limit: 20`, rutas informa `total: 57`, `shown: 20` y `truncated: true`.
- Binario instalado en `/Users/MoreFer/.local/bin/codebase-memory-mcp` y
  comprobado contra el índice `full` el 13 de julio de 2026.

### PR prioritaria C: capas basadas en evidencia

- [x] Evitar presentar fan-in/fan-out como una clasificación arquitectónica
  categórica; incluir `confidence` y las evidencias utilizadas.
- [x] Reconocer raíces de aplicación y proyectos mediante `main.ts`,
  `Program.cs`, `angular.json`, `package.json` y `.csproj`.
- [x] Extraer de `.csproj` al menos `Sdk`, `OutputType`, `TargetFramework`,
  `ProjectReference` y recursos embebidos relevantes para distinguir código,
  proyectos y assets.
- [x] Clasificar dentro de cada raíz de aplicación/proyecto antes de usar el
  grafo de llamadas; no mezclar automáticamente frontend, backend, scripts y
  herramientas del monorepo.
- [x] Devolver `unknown` o baja confianza cuando no exista evidencia suficiente,
  en lugar de forzar `entry`, `api`, `core`, `leaf` o `internal`.
- [x] Añadir fixtures sintéticos de monorepo Angular + ASP.NET Core con proyectos
  ejecutables, librerías, referencias, assets y capas ambiguas.
- [x] Comprobar que `get_architecture` pueda justificar la arquitectura sin
  requerir una lectura manual posterior de los archivos de arranque y proyecto.

Rama: `feat/architecture-evidence-layers` (eliminada tras el merge)
PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/23
Merge: squash en `main` el 20 de julio de 2026, commit `0f1f3d1`

Implementado:

- Detección de raíces de proyecto desde nodos `File` marcadores (`Program.cs`,
  `main.ts`, `angular.json`, `package.json`, `*.csproj`) en `arch_collect_roots`.
- Lectura de `.csproj` desde disco en `arch_read_csproj` para extraer `Sdk`,
  `OutputType` y `TargetFramework`; los proyectos con `OutputType=Library` se
  clasifican como `library` en lugar de backend.
- Clasificador `classify_layer_evidence` que asigna `layer`, `reason`,
  `confidence` (`high` | `medium` | `low` | `unknown`) y `evidence` usando las
  raíces detectadas, las rutas y los entry points; el fan-in/fan-out se conserva
  solo como fallback de baja confianza cuando no hay marcadores.
- Serialización JSON de `confidence` y `evidence` en `handle_get_architecture`.
- Fixture sintético de monorepo Angular + ASP.NET Core + librería .NET +
  script en `arch_layers_evidence`.

Validación local:

- Suite dirigida `store_arch`: 55 pruebas pasaron (incluye `arch_layers_evidence`).
- Suite dirigida `mcp`: 146 pruebas pasaron.
- `make -j3 -f Makefile.cbm lint-ci` (clang-format + cppcheck + NOLINT): correcto.
- `scripts/test.sh`: todas las pruebas pasaron.
- `api` con `Program.cs` + rutas → `api`, `confidence: high`, evidencia cita
  `Program.cs`.
- `app` con `main.ts` y sin rutas → `entry`, `confidence: high`, evidencia cita
  `main.ts`.
- `lib` con `Lib.csproj` `OutputType=Library` → `core`, `confidence: medium`,
  evidencia cita `Lib.csproj` y `net10.0`.
- `tools` sin marcadores → fallback de `confidence: low` con evidencia
  «no project markers».

Validación privada `ppi-ntv` (binario nuevo instalado el 20 de julio de 2026):

- Índice `full`: 13.233 nodos y 35.212 aristas.
- `get_architecture` devuelve 9 capas con `confidence` y `evidence`:
  `ntv-frontend` y `ntv-testhost` → `entry`/high («entry marker: main.ts»);
  paquete de rutas → `api`/high; `ntv-backend`, `ntv-hangfire` y
  `unfinished-customer-interactions` → `internal`/medium (evidencia cita
  `Program.cs` y la raíz del proyecto); 3 paquetes sin marcadores →
  `confidence: low`. Desglose 3 high / 3 medium / 3 low.
- Línea base conservada: 73 rutas (`truncated: false`), 7 entry points,
  `DATA_FLOWS` = 20, `HTTP_CALLS` = 21, `HANDLES` = 46, `LOADS_ASSET` = 3.
- El resumen se justifica solo, sin contraste manual con `main.ts`/`Program.cs`/
  `.csproj`.

### Orden obligatorio

1. PR prioritaria A: ruido de minificados. ✅ fusionada (PR 6).
2. PR prioritaria B: completitud de rutas y handlers. ✅ fusionada (PR 8).
3. PR prioritaria C: clasificación de capas basada en evidencia. ✅ fusionada (PR 23).
4. PR 6 (Angular Router y lazy loading). ✅ fusionada (PR 25).
5. PR 11 (invocaciones genéricas C#). ✅ fusionada (PR #29, squash `3749ae9`).
6. PR 12 (DI de .NET, depende de PR 11). ← siguiente.

## Trabajo completado

### PR 1: rutas ASP.NET Core

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/1  
Rama: `feat/aspnet-core-routes`  
Commit: `23bf852`

Implementado:

- Reconocimiento de `[Route]`.
- Reconocimiento de `[HttpGet]`, `[HttpPost]`, `[HttpPut]`, `[HttpDelete]` y
  `[HttpPatch]`.
- Soporte para rutas relativas y atributos HTTP sin argumento.
- Composición de ruta de controlador y acción.
- Resolución de `[controller]` desde el nombre de la clase, no desde el archivo.
- Resolución de `[action]`.
- Materialización de versiones declaradas con `[ApiVersion]`.
- Soporte para múltiples listas de atributos, por ejemplo `[Authorize]` seguido
  de `[HttpGet]`.
- Fixtures sintéticos de extracción y aristas `HANDLES`.

Validación:

- El índice privado pasó de 13 a 51 nodos `Route`.
- Se detectaron 38 rutas MVC ASP.NET nuevas.
- Se generaron 41 aristas `HANDLES`.
- `make -j3 -f Makefile.cbm lint-ci` pasó.
- Las suites dirigidas pasaron.
- Suite completa: 5.985 pruebas pasaron; dos timeouts transitorios del supervisor
  pasaron al repetir `build/c/test-runner mcp`.

Archivos principales:

- `internal/cbm/extract_defs.c`
- `tests/test_extraction.c`
- `tests/test_edge_types_probe.c`

### PR 2: Angular HttpClient

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/2  
Rama: `feat/angular-http-client`  
Commit: `b34c593`

Implementado:

- Reconocimiento de `HttpClient` inyectado como propiedad del constructor.
- Reconocimiento de campos creados con `inject(HttpClient)`.
- Exclusión de receptores `.get()`/`.post()` que no sean `HttpClient`.
- Soporte para template strings.
- Propagación local conservadora de variables como `const apiURL = ...`.
- Extracción del path cuando la URL base es dinámica.
- Canonicalización de `${param}` como `{}`.
- Eliminación de query strings y fragmentos para emparejar rutas.
- Clasificación de llamadas externas de Angular como `HTTP_CALLS`, aunque
  `@angular/common/http` no esté indexado.
- Fixtures positivos y negativos sintéticos.

Validación:

- `HTTP_CALLS` en el índice privado aumentó de 2 a 14.
- Se detectan correctamente las llamadas directas de los servicios Angular.
- `make -j3 -f Makefile.cbm lint-ci` pasó.
- Las suites `extraction` y `edge_types_probe` pasaron.
- Suite completa: 5.986 pruebas pasaron; un timeout transitorio del supervisor
  pasó al repetir `build/c/test-runner mcp`.

Archivos principales:

- `internal/cbm/extract_calls.c`
- `tests/test_extraction.c`
- `tests/test_edge_types_probe.c`

### PR 3: wrappers HTTP y convergencia Angular-ASP.NET

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/3  
Rama: `feat/angular-http-wrappers`  
Commit: `9eb3343`

Implementado:

- Propagación conservadora de URLs a través de wrappers `HttpClient` de una capa.
- Atribución de `HTTP_CALLS` al método público consumidor.
- Rechazo de URLs mutables, wrappers ambiguos y múltiples sinks.
- Matching case-insensitive y de parámetros concretos solo para productores ASP.NET.
- Preferencia por rutas estáticas específicas frente a plantillas parametrizadas.
- Paridad entre pipelines secuencial y paralelo, sin rutas fallback duplicadas.

Validación:

- 20/20 llamadas Angular de producción enlazadas con su acción ASP.NET.
- 0 enlaces del frontend hacia endpoints internos y 0 rutas de assets como HTTP.
- `trace_path` en modo `cross_service` atraviesa caller, ruta y controlador.
- Suite completa: 5.994 pruebas pasaron y una se omitió.
- Build de producción, `cppcheck`, formato y checks de watchdog/seguridad correctos.

Archivos principales:

- `internal/cbm/extract_calls.c`
- `src/pipeline/pass_calls.c`
- `src/pipeline/pass_parallel.c`
- `src/pipeline/pass_route_nodes.c`
- `tests/test_extraction.c`
- `tests/test_edge_types_probe.c`

## Estado instalado

Instalación validada el 10 de julio de 2026:

- Binario instalado: `/Users/MoreFer/.local/bin/codebase-memory-mcp`.
- Backup:
  `/Users/MoreFer/.local/bin/codebase-memory-mcp.backup-0.8.1-20260710T124925`.
- Índice definitivo: `Users-MoreFer-dev-fielmann-ag-ppi-ntv`.
- Resultado: 10.939 nodos, 30.961 aristas, 2 nodos `Asset`, 3 aristas
  `LOADS_ASSET` y 20 llamadas Angular enlazadas con acciones ASP.NET.
- Cursor se recargó y la nueva funcionalidad quedó confirmada mediante MCP.

## Roadmap de semántica Angular + .NET

### Decisiones de modelo

- Conservar `Class`, `Method`, `Interface` y `Type` como nodos base, añadiendo
  propiedades de framework cuando sea suficiente.
- Crear nodos nuevos solo para conceptos sin equivalente claro:
  `NavigationRoute` y `FeatureFlag`.
- No reutilizar `Route` para Angular Router. `Route` seguirá reservado a HTTP;
  la navegación cliente no debe contaminar `HTTP_CALLS`, `DATA_FLOWS` ni
  `cross_service`.
- Implementar toda emisión basada en llamadas con paridad entre
  `src/pipeline/pass_calls.c` y `src/pipeline/pass_parallel.c`.
- Registrar cada nodo y arista nuevos en el esquema MCP, consultas,
  documentación y pruebas de convergencia.
- Mantener cada capacidad en una PR pequeña, con fixtures sintéticos y sin
  reglas hard-coded para `ppi-ntv`.

### Track Angular

#### PR 5: componentes standalone y metadatos Angular

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/5  
Rama: `feat/angular-metadata`  
Commit: `180de21`

- [x] Interpretar argumentos literales de `@Component`, `@Directive`, `@Pipe`,
  `@Injectable` y `@NgModule`.
- [x] Guardar `angular_kind`, `selector`, `standalone`, `templateUrl` y
  `styleUrls` en las definiciones existentes.
- [x] Resolver imports standalone mediante la infraestructura actual de imports.
- [x] Rechazar configuraciones dinámicas o ambiguas.

Zonas principales:

- `internal/cbm/extract_defs.c`
- `internal/cbm/extract_imports.c`
- `src/pipeline/pass_definitions.c`
- `tests/test_extraction.c`

Validación:

- Fixtures positivos, negativos, ambiguos y de deduplicación.
- Paridad secuencial/paralela para propiedades y aristas `IMPORTS`.
- Arrays completos expuestos por `search_graph` y `query_graph`.
- Suite dirigida, `lint-ci` y `scripts/test.sh` correctos.
- Índice privado reconstruido desde cero: 30 componentes, 1 directiva,
  31 injectables y 24 imports standalone resueltos.
- Línea base conservada: 20/20 llamadas Angular con handler ASP.NET, 2 assets y
  3 aristas `LOADS_ASSET`.
- PR fusionada en `main` el 10 de julio de 2026.

#### PR 6: Angular Router y lazy loading

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/25  
Rama: `feat/angular-router` (eliminada tras el merge)  
Merge: commit `ce1bd12` en `main` el 20 de julio de 2026 (feature commit `514003d`)

- [x] Extraer arrays `Routes` (y recursar `children`); `provideRouter` y
  `RouterModule.forRoot`/`forChild` quedan fuera de este alcance.
- [x] Crear nodos `NavigationRoute` con path canónico (QN `__navroute__/<path>`).
- [x] Emitir `ROUTES_TO`, `REDIRECTS_TO`, `LAZY_LOADS` y `DECLARES_ROUTE` hacia
  componentes, módulos o rutas destino.
- [x] Resolver `loadComponent` y `loadChildren` solo cuando `import()` sea
  estático, con fallback para imports relativos de dotted-stem
  (`./branch-zone/branch-zone.routes`) que el resolver estándar mal-stripa.
- [x] Mantener la navegación fuera del matching HTTP y de `cross_service`.

Zonas principales:

- `internal/cbm/extract_angular_routes.c` (nuevo extractor)
- `src/pipeline/pass_route_nodes.c` (emisión de nodos y aristas)
- `src/pipeline/pass_definitions.c` y `src/pipeline/pass_parallel.c` (paridad)
- `tests/test_extraction.c` y `tests/test_parallel.c`

Validación:

- Test de extractor `extract_angular_routes_array` (rutas raíz/parámetro,
  `loadComponent`, `loadChildren`, `redirectTo`, `children` anidados).
- Test de paridad secuencial/paralela `parallel_angular_nav_routes_parity`
  (cuenta de nodos `NavigationRoute` y los 4 tipos de arista coincidentes).
- `scripts/lint.sh --ci` (clang-format + cppcheck): correcto.
- `scripts/test.sh`: 6.011 pruebas pasaron, 1 omitida.
- Índice privado `ppi-ntv` reconstruido desde cero: 13.238 nodos y 35.227 aristas;
  5 nodos `NavigationRoute` (`/`, `branch-zone`, `bouncer`, `edit-page`, `edit`),
  2 `ROUTES_TO`, 3 `REDIRECTS_TO`, 1 `LAZY_LOADS` (`branch-zone` →
  `branch-zone.routes.ts` vía fallback dotted-stem), 9 `DECLARES_ROUTE`.
  Reconciliación línea por línea con `app.routes.ts` y `branch-zone.routes.ts`:
  sin nodos ni aristas faltantes o sobrantes; el dedup es consistente
  (`DECLARES_ROUTE` no se deduplica entre archivos porque cada `const routes` es
  un nodo `Variable` distinto; `ROUTES_TO`/`REDIRECTS_TO` sí se deduplican por
  nodo `NavigationRoute` compartido).
- Línea base conservada: navegación cliente no contamina `HTTP_CALLS`,
  `DATA_FLOWS` ni `cross_service`.

Follow-up PR 27 (merge commit `fbf6ab4`): registro de los tipos nuevos en
`README.md` (feature blurb, `Edge types (selected)`, `Node Labels` y lista
completa de `Edge Types`), en la ayuda del CLI (`src/cli/cli.c`) y en el
histograma de contratos (`ALL_EDGE_TYPES` en `tests/test_lang_contract.c`).
Esto completa el criterio de aceptación del issue #11 («registrar los tipos
nuevos en esquema, contratos y README»). Sin cambios de comportamiento del
grafo; solo superficie de docs/contratos.

Issue #11 cerrado tras el follow-up. Issues #7, #9 y #10 (arquitectura,
PRs A/B/C) cerrados también al verificar su trabajo ya fusionado.

#### PR 7: guards Angular

- [ ] Reconocer `canActivate`, `canMatch` y `canDeactivate`.
- [ ] Emitir `GUARDS` desde clases o funciones hacia `NavigationRoute`.
- [ ] Guardar el índice del array como propiedad sin imponer ejecución ordenada.
- [ ] No crear un enlace único cuando el guard no pueda resolverse sin
  ambigüedad.

#### PR 8: DI e interceptores Angular

- [ ] Generalizar la detección actual de `inject(HttpClient)` a providers
  literales.
- [ ] Reconocer `providedIn`, `bootstrapApplication`, `provideHttpClient` y
  `withInterceptors`.
- [ ] Emitir `PROVIDES` e `INTERCEPTS` con token, `multi` y orden.
- [ ] Diferir factories y arrays dinámicos.
- [ ] Mantener estas aristas fuera de `cross_service` por defecto.

Zonas principales:

- `internal/cbm/extract_calls.c`
- `internal/cbm/extract_defs.c`
- `src/pipeline/pass_calls.c`
- `src/pipeline/pass_parallel.c`

#### PR 9: templates Angular

- [ ] Vincular `templateUrl` e inline `template` con su componente.
- [ ] Extraer selectores y bindings.
- [ ] Extraer control flow: `*ngIf`, `@if`, `@for` y `@switch`.
- [ ] Usar un extractor acotado o una gramática vendorizada, sin añadir
  dependencias en runtime.
- [ ] Dividir en PR 9a y 9b si selector/bindings y control flow no caben en una
  PR revisable.

Zonas principales:

- `internal/cbm/lang_specs.c`
- `src/discover/language.c`
- un nuevo `internal/cbm/extract_angular_template.c`
- `src/pipeline/pass_semantic_edges.c`

#### PR 10: assets declarativos

- [ ] Reutilizar `Asset` y `LOADS_ASSET` para `src`, `href`, `styleUrls` y
  CSS/SCSS `url(...)`.
- [ ] Añadir `source: template|stylesheet`.
- [ ] Resolver paths relativos de forma conservadora.
- [ ] Seguir excluyendo assets de rutas HTTP y de `cross_service`.

Zonas principales:

- `internal/cbm/extract_calls.c`
- `src/pipeline/pass_route_nodes.c`
- `tests/test_edge_types_probe.c`

### Track .NET y convergencia

#### PR 11: tipos genéricos en invocaciones C#

PR: https://github.com/fernandofielmann/codebase-memory-mcp-angular-dotnet/pull/29  
Rama: `feat/csharp-generic-invocations` (eliminada tras el merge)  
Merge: squash en `main` el 20 de julio de 2026, commit `3749ae9` (feature commit `324c757`)

- [x] Preservar los argumentos de `generic_name` en `CBMCall`.
- [x] Cubrir invocaciones como `AddScoped<TService,TImpl>` y tipos de respuesta.
- [x] Añadir fixtures positivos y negativos de extracción antes de consumir
  esta información en otros pases.

Implementado:

- Nuevos campos `generic_args` (cadena comma-joined, arena) y
  `generic_arg_count` en `CBMCall` (`internal/cbm/cbm.h`).
- `extract_callee_from_fields` resuelve ahora el callee para invocaciones
  genéricas C# tanto bare (`generic_name` como `function`) como member
  (`member_access_expression` cuyo `name` es `generic_name`), devolviendo un
  callee limpio («AddScoped» / «recv.AddScoped») sin `<...>`. La gramática C#
  de tree-sitter expone el identificador genérico como primer hijo `identifier`
  de `generic_name`, no vía el campo `name`.
- `extract_call_generic_args` captura los argumentos de tipo desde
  `type_argument_list`, unidos por coma y conservando el texto fuente de cada
  argumento para que los genéricos anidados (`Dictionary<string,int>`) round-trip
  sin cambios; escribe el conteo en `CBMCall.generic_arg_count`.
- `handle_calls` rellena los nuevos campos. Superficie sólo de extracción,
  consumida por pases posteriores (PR 12 DI).

Zonas principales:

- `internal/cbm/cbm.h`
- `internal/cbm/extract_calls.c`
- `tests/test_extraction.c`

Validación local:

- Test `csharp_generic_invocations`: callee member limpio (sin `<`),
  `generic_arg_count == 2`, `generic_args == "IOrderService,OrderService"`;
  bare `DoThing<int,string>()` ya no se descarta y conserva `int,string`;
  hermano no genérico con `count == 0` y `generic_args == NULL`; las tres
  formas de registro DI (AddScoped/AddSingleton/AddTransient) con dos
  argumentos de tipo.
- Suite dirigida `extraction`: 220 pruebas pasaron.
- Suite dirigida `edge_types_probe`: 63 pruebas pasaron.
- `make -f Makefile.cbm lint-ci` (clang-format + cppcheck + NOLINT): correcto.
- `scripts/test.sh` (suite completa): 6.012 pruebas pasaron, 1 omitida.

Validación privada `ppi-ntv` (binario nuevo en `build/c/`, no instalado):

- Índice `full`: 13.238 nodos y 35.339 aristas, 0 errores.
- Probe sobre `DependencyInjectionExtension.cs` real: 37 llamadas genéricas
  capturadas con `generic_args` correctos, p. ej.
  `AddScoped<ISqlClient,SqlClient>`, `AddSingleton<ICountryProvider,CountryProvider>`,
  `IUnitOfWork<NtvContext>,UnitOfWork<NtvContext>` (genérico anidado),
  `AddDbContextPool<NtvContext>`, `GetRequiredService<IServiceScopeFactory>`,
  `GetService<NtvContext>`.
- Hallazgo para PR 12: el `callee_name` de llamadas encadenadas es el texto
  completo de la cadena (multi-línea); convendrá usar el método trailing +
  `generic_args` en PR 12. Es comportamiento pre-existente, no regresión de
  PR 11.

Pendiente: instalar el binario nuevo en `~/.local/bin/` (el MCP server sigue
usando el binario anterior sin PR 11).

#### PR 12: DI de .NET

- [ ] Reconocer `AddTransient`, `AddScoped`, `AddSingleton` y equivalentes no
  genéricos sobre `IServiceCollection`.
- [ ] Resolver interfaz e implementación contra el registro de tipos.
- [ ] Emitir `RESOLVES_TO` con `lifetime` y `CONFIGURES` desde el método de
  registro.
- [ ] Cubrir registros múltiples y rechazar factories no resolubles.

#### PR 13: MediatR

- [ ] Relacionar `IMediator.Send` y `Publish` con commands, queries y
  notifications.
- [ ] Resolver implementaciones únicas de `IRequestHandler` y
  `INotificationHandler`.
- [ ] Emitir `DISPATCHES` y `HANDLES_MESSAGE`.
- [ ] Permitir una traza explícita controlador → mensaje → handler.
- [ ] Omitir el enlace único cuando haya handlers ambiguos.

#### PR 14: contratos TypeScript-C#

- [ ] Ejecutar un pase posterior al matching de rutas.
- [ ] Analizar solo nodos HTTP compartidos por `HTTP_CALLS` y `HANDLES`.
- [ ] Extraer tipos nombrados de body y respuesta en ambos lados.
- [ ] Emitir `MATCHES_CONTRACT` con `direction: request|response`.
- [ ] No usar similitud global de nombres.
- [ ] Omitir `any`, `unknown`, objetos anónimos y endpoints ambiguos.

Zonas principales:

- `internal/cbm/extract_type_refs.c`
- `internal/cbm/extract_type_assigns.c`
- un nuevo `src/pipeline/pass_contracts.c`
- `src/pipeline/pass_semantic_edges.c`
- `tests/test_convergence_probe.c`

#### PR 15: feature flags cross-stack

- [ ] Crear nodos `FeatureFlag` por clave literal exacta.
- [ ] Emitir `DEFINES_FLAG` y `READS_FLAG` desde constantes, configuración,
  guards, servicios y templates.
- [ ] Empezar por claves literales de `IConfiguration`/`IOptions` y accesos
  frontend literales.
- [ ] Ofrecer traza explícita de flags antes de incorporarla a modos generales.

## Dependencias y orden recomendado

1. PR 5 completada; PR 6 (Angular Router) completada. PR 11 (invocaciones
   genéricas C#) fusionada (PR #29). PR 7/PR 8 ya tienen satisfecha la
   dependencia sobre PR 6.
2. Angular: PR 6 ✅ → PR 7 y PR 8 → PR 9 → PR 10; las dependencias sobre PR 5
   ya están satisfechas.
3. .NET: PR 11 ✅ (fusionada) → PR 12 → PR 13.
4. PR 14 comienza cuando PR 11 y el matching HTTP sigan estables.
5. PR 15 cierra el roadmap después de PR 9, PR 12 y PR 14.

## Validación obligatoria por PR

- [ ] Añadir positivos, negativos, ambigüedad y deduplicación en
  `tests/test_extraction.c`.
- [ ] Añadir pruebas end-to-end y paridad secuencial/paralela en
  `tests/test_edge_types_probe.c`.
- [ ] Actualizar contratos de aristas en `tests/test_lang_contract.c` y
  `tests/test_convergence_probe.c`.
- [ ] Actualizar esquema MCP en `src/mcp/mcp.c` y documentación en `README.md`.
- [ ] Ejecutar:

```bash
build/c/test-runner extraction edge_types_probe
make -j3 -f Makefile.cbm lint-ci
scripts/test.sh
```

- [ ] Reindexar `ppi-ntv` desde cero y conservar la línea base:
  20/20 enlaces Angular→ASP.NET y 0 clasificaciones HTTP falsas de assets o
  navegación.
- [ ] Añadir métricas privadas de cada capacidad nueva sin publicar nombres ni
  código de `ppi-ntv`.
