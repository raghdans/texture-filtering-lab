# יומן הפיתוח — Texture Filtering Lab

זהו תיעוד של העבודה שעשיתי על הפרויקט. לא בניתי את הכול בבת אחת: בכל
איטרציה הוספתי אפשרות אחת, בניתי והרצתי את התוכנית, בדקתי את התוצאה ורק אז
שמרתי commit. נעזרתי ב־AI להסברים, הצעות ועזרה באיתור תקלות, אבל עברתי על
השינויים ובדקתי אותם בעצמי.

## איטרציה 1 — Nearest Neighbor

המטרה הראשונה הייתה להציג טקסטורת לוח שחמט בשיטת דגימה אחת בלבד.

**ה־prompt:**

> Create only the first iteration of a C++17 OpenGL 3.3 texture-filtering lab.
> Display a square with UV coordinates and a procedural checkerboard texture.
> Use nearest-neighbor filtering only. Do not implement later features yet.

בהתחלה CMake לא מצא compiler בגלל כפילות של Path/PATH. לאחר מכן GLAD לא
נבנה בגלל Jinja2. פתרתי את שתי הבעיות, אך חלון OpenGL נשאר שחור. בדיקות
פיקסלים הראו שהציור קיים ב־back buffer אך אינו מוצג ב־front buffer במחשב
שלי. גם ניסיון ללא double buffering לא פתר זאת. מאחר ש־MiniFB מהתרגיל הקודם
עבד באותו מחשב, עברתי להצגת framebuffer של ה־CPU באמצעות MiniFB, ואת
אלגוריתמי הדגימה מימשתי ידנית.

בבנייה הראשונה היה חסר `MiniFB_cpp.cpp`. לאחר הוספתו התוכנית קרסה בגלל
stack overflow, כי framebuffer בגודל כ־2.5MB היה משתנה מקומי. העברתי אותו
לאחסון סטטי. בסוף התקבל לוח שחמט כחול־לבן עם גבולות חדים.

**Commit:** `7999057`

## איטרציה 2 — Bilinear ומעבר בין השיטות

**ה־prompt:**

> Starting from the tested nearest-neighbor CPU sampler, add a bilinear sampler
> as a separate function. Interpolate the four surrounding texels per RGB
> channel, clamp boundaries, and add keys 1 and 2 to switch methods.

הוספתי דגימת Bilinear ידנית. בהרצה הראשונה המקש 2 לא הגיב, ולכן שיניתי את
טיפול האירועים והוספתי keyboard callback. אחר כך הצבע הכחול הפך לחום בגלל
סדר שגוי של ערוצי RGB ב־MiniFB. תיקנתי את ה־bit shifts. בדקתי ש־1/N מציג
גבולות חדים וש־2/B מציג מעברים חלקים, בלי לשנות את צבעי המקור.

**Commit:** `01965ff`

## איטרציה 3 — השוואה זו לצד זו

**ה־prompt:**

> Keep the two tested sampling functions unchanged. Add a mode that draws
> nearest-neighbor on the left and bilinear on the right, with a gap between
> them. Use 3 or S and retain the existing modes.

יצרתי פונקציה כללית בשם `renderPanel` והצגתי את שתי השיטות באותו חלון. כך
אפשר לראות מיד את הגבולות החדים משמאל ואת הערבוב החלק מימין.

**Commit:** `f7336dd`

## איטרציה 4 — הקטנה ו־aliasing

**ה־prompt:**

> Add a dense 64 x 64 checkerboard and sample it onto a 32 x 32 logical grid
> using nearest-neighbor on the left and bilinear on the right. Enlarge the
> results for inspection. Do not implement mipmaps yet.

יצרתי טקסטורה צפופה ודגמתי אותה לתמונה קטנה יותר. Nearest יצר אזורים גדולים
שלא היו במקור, ו־Bilinear יצר מעברי צבע רחבים. הבדיקה המחישה ש־Bilinear לבדו
אינו פותר aliasing בזמן הקטנה.

**Commit:** `8a31a67`

## איטרציה 5 — רמת Mipmap ראשונה

**ה־prompt:**

> Build one 32 x 32 mipmap level from the dense texture. Average each 2 x 2
> source block and compare the result with direct bilinear minification.

יצרתי רמה בגודל 32×32 באמצעות ממוצע של ארבעה טקסלים לכל טקסל חדש. Bilinear
ישיר הציג gradient לא רצוי, ואילו ה־Mipmap הציג כחול־אפור אחיד. זו התוצאה
הנכונה, כי כל בלוק 2×2 מכיל שתי משבצות כחולות ושתי משבצות לבנות.

**Commit:** `b78afaa`

## איטרציה 6 — שרשרת Mipmaps מלאה

**ה־prompt:**

> Generalize the 2 x 2 averaging step into a complete mipmap chain. Build each
> level from the preceding level until 1 x 1 and display all seven levels.

הרחבתי את החישוב לכל הרמות מ־64×64 עד 1×1. בכל שלב הרוחב והגובה קטנים בחצי.
בדקתי שהפרטים נעלמים בהדרגה ושהרמה האחרונה מכילה את הצבע הממוצע.

**Commit:** `ee65d31`

## איטרציה 7 — בחירת רמה אוטומטית

**ה־prompt:**

> Select a mipmap level from the output size. Add a view for output sizes 64,
> 32, 16, and 8 in a 2 x 2 grid. Keep all earlier modes.

כתבתי את `chooseMipLevel`, שבוחרת את הרמה הקרובה לגודל הפלט בלי לרדת לרמה
קטנה מדי. ה־console אישר את הבחירות 64→64, 32→32, 16→16 ו־8→8.

**Commit:** `c3393ad`

## איטרציה 8 — שינוי גודל בזמן אמת

**ה־prompt:**

> Add an interactive mode entered with 8 or I. Use Up and Down to double or
> halve the output size between 8 and 512, and select the mip level automatically.

הוספתי מצב שבו החצים משנים את גודל הפלט. בדקתי את הרצף 256, 128, 64 ו־32.
כאשר הפלט הגיע ל־32, התוכנית עברה אוטומטית מרמת 64 לרמת 32.

**Commit:** `dcabd0c`

## איטרציה 9 — הקטנה בפרספקטיבה

**ה־prompt:**

> Draw two perspective-style textured surfaces. Use direct bilinear sampling
> on the left and automatic mip selection per scanline on the right. Add key 9/R.

ציירתי שני משטחים שמתרחבים מלמעלה למטה. הצד השמאלי תמיד משתמש ב־Bilinear
מהטקסטורה המקורית. הצד הימני בוחר רמת Mipmap לפי רוחב כל שורה. בדקתי ששני
המשטחים מוצגים נכון ושכל המצבים הקודמים עדיין זמינים.

**Commit:** `a2b5840`

## איטרציה 10 — כותרות בתוך החלון

**ה־prompt:**

> Add a small built-in bitmap font for NEAREST, BILINEAR, and MIPMAP. Draw the
> labels above the relevant comparisons without adding another dependency.

הוספתי גופן bitmap קטן בגודל 5×7 שנכתב ישירות ל־framebuffer. בדקתי שהכותרות
מופיעות מעל השיטות הנכונות, במיוחד BILINEAR משמאל ו־MIPMAP מימין במצב
הפרספקטיבה.

**Commit:** `9b4db5c`

## סיכום אישי של התהליך

בסוף ביצעתי בניית Release ובדקתי את כל תשעת מצבי התצוגה. במהלך העבודה היו
ניסיונות שלא הצליחו, במיוחד חלון OpenGL השחור, ולכן שיניתי את דרך הצגת
הפיקסלים. בהמשך תיקנתי גם בעיות זיכרון, קלט וסדר צבעים. כל תיקון נבדק לפני
שהמשכתי לשלב הבא. התוצאה הסופית מאפשרת לראות בפועל את ההבדלים בין Nearest
Neighbor, Bilinear ו־Mipmaps ואת ההשפעה שלהם בזמן הגדלה והקטנה.
