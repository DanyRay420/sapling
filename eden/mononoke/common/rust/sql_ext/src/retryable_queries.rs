/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

use std::future::Future;
use std::time::Duration;

use anyhow::Result;
use retry::retry;
use retry::RetryLogic;
use tunables::tunables;

const RETRY_ATTEMPTS: usize = 2;

#[macro_export]
macro_rules! queries_with_retry {
    () => {};

    (
        $vi:vis read $name:ident (
            $( $pname:ident: $ptype:ty ),* $(,)*
            $( >list $lname:ident: $ltype:ty )*
        ) -> ($( $rtype:ty ),* $(,)*) { $q:expr }
        $( $rest:tt )*
    ) => {
        $crate::queries_with_retry! {
            $vi read $name (
                $( $pname: $ptype, )*
                $( >list $lname: $ltype )*
            ) -> ($( $rtype ),*) { mysql($q) sqlite($q) }
            $( $rest )*
        }
    };

    (
        $vi:vis read $name:ident (
            $( $pname:ident: $ptype:ty ),* $(,)*
            $( >list $lname:ident: $ltype:ty )*
        ) -> ($( $rtype:ty ),* $(,)*) { mysql($mysql_q:expr) sqlite($sqlite_q:expr) }
        $( $rest:tt )*
    ) => {
        $crate::_macro_internal::paste::item! {
            $crate::_macro_internal::queries! {
                pub read [<$name Impl>] (
                    $( $pname: $ptype, )*
                    $( >list $lname: $ltype )*
                ) -> ($( $rtype ),*) { mysql($mysql_q) sqlite($sqlite_q) }
            }

            #[allow(non_snake_case)]
            $vi mod $name {
                #[allow(unused_imports)]
                use super::*;

                use $crate::_macro_internal::*;

                // Not possible to retry query with transaction
                #[allow(unused_imports)]
                pub use [<$name Impl>]::query_with_transaction;

                #[allow(dead_code)]
                pub async fn query(
                    connection: &Connection,
                    $( $pname: & $ptype, )*
                    $( $lname: & [ $ltype ], )*
                ) -> Result<Vec<($( $rtype, )*)>> {
                    query_with_retry(
                        || [<$name Impl>]::query(connection, $( $pname, )* $( $lname, )*),
                    ).await
                }
            }

            $crate::queries_with_retry! { $( $rest )* }
        }
    };

    (
        $vi:vis write $name:ident (
            values: ($( $vname:ident: $vtype:ty ),* $(,)*)
            $( , $pname:ident: $ptype:ty )* $(,)*
        ) { $qtype:ident, $q:expr }
        $( $rest:tt )*
    ) => {
        $crate::queries_with_retry! {
            $vi write $name (
                values: ( $( $vname: $vtype ),* )
                $( , $pname: $ptype )*
            ) { $qtype, mysql($q) sqlite($q) }
            $( $rest )*
        }
    };

    (
        $vi:vis write $name:ident (
            values: ($( $vname:ident: $vtype:ty ),* $(,)*)
            $( , $pname:ident: $ptype:ty )* $(,)*
        ) { $qtype:ident, mysql($mysql_q:expr) sqlite($sqlite_q:expr) }
        $( $rest:tt )*
    ) => {
        $crate::_macro_internal::paste::item! {
            $crate::_macro_internal::queries! {
                pub write [<$name Impl>] (
                    values: ( $( $vname: $vtype ),* )
                    $( , $pname: $ptype )*
                ) { $qtype, mysql($mysql_q) sqlite($sqlite_q) }
            }

            #[allow(non_snake_case)]
            $vi mod $name {
                #[allow(unused_imports)]
                use super::*;

                use $crate::_macro_internal::*;

                // Not possible to retry query with transaction
                #[allow(unused_imports)]
                pub use [<$name Impl>]::query_with_transaction;

                #[allow(dead_code)]
                pub async fn query(
                    connection: &Connection,
                    values: &[($( & $vtype, )*)],
                    $( $pname: & $ptype ),*
                ) -> Result<WriteResult> {
                    query_with_retry(
                        || [<$name Impl>]::query(connection, values $( , $pname )* ),
                    ).await
                }
            }

            $crate::queries_with_retry! { $( $rest )* }
        }
    };

    (
        $vi:vis write $name:ident (
            $( $pname:ident: $ptype:ty ),* $(,)*
            $( >list $lname:ident: $ltype:ty )*
        ) { $qtype:ident, $q:expr }
        $( $rest:tt )*
    ) => {
        $crate::queries_with_retry! {
            $vi write $name (
                $( $pname: $ptype, )*
                $( >list $lname: $ltype )*
            ) { $qtype, mysql($q) sqlite($q) }
            $( $rest )*
        }
    };

    (
        $vi:vis write $name:ident (
            $( $pname:ident: $ptype:ty ),* $(,)*
            $( >list $lname:ident: $ltype:ty )*
        ) { $qtype:ident, mysql($mysql_q:expr) sqlite($sqlite_q:expr) }
        $( $rest:tt )*
    ) => {
        $crate::_macro_internal::paste::item! {
            $crate::_macro_internal::queries! {
                pub write [<$name Impl>] (
                    $( $pname: $ptype, )*
                    $( >list $lname: $ltype )*
                ) { $qtype, mysql($mysql_q) sqlite($sqlite_q) }
            }

            #[allow(non_snake_case)]
            $vi mod $name {
                #[allow(unused_imports)]
                use super::*;

                use $crate::_macro_internal::*;

                // Not possible to retry query with transaction
                #[allow(unused_imports)]
                pub use [<$name Impl>]::query_with_transaction;

                #[allow(dead_code)]
                pub async fn query(
                    connection: &Connection,
                    $( $pname: & $ptype, )*
                    $( $lname: & [ $ltype ], )*
                ) -> Result<WriteResult> {
                    query_with_retry(
                        || [<$name Impl>]::query(connection, $( $pname, )* $( $lname, )*),
                    ).await
                }
            }

            $crate::queries_with_retry! { $( $rest )* }
        }
    };

}

#[cfg(fbcode_build)]
/// See https://fburl.com/sv/uk8w71td for error descriptions
fn retryable_mysql_errno(errno: u32) -> bool {
    match errno {
        // Admission control errors
        // Safe to retry on writes as well as the query didn't even start
        1914..=1916 => true,
        _ => false,
    }
}

#[cfg(fbcode_build)]
fn should_retry_mysql_query(err: &anyhow::Error) -> bool {
    use mysql_client::MysqlError;
    use MysqlError::*;
    match err.downcast_ref::<MysqlError>() {
        Some(ConnectionOperationError { mysql_errno, .. })
        | Some(QueryResultError { mysql_errno, .. }) => retryable_mysql_errno(*mysql_errno),
        _ => false,
    }
}

#[cfg(not(fbcode_build))]
fn should_retry_mysql_query(err: &anyhow::Error) -> bool {
    false
}

pub async fn query_with_retry<T, Fut>(mut do_query: impl FnMut() -> Fut + Send) -> Result<T>
where
    T: Send + 'static,
    Fut: Future<Output = Result<T>>,
{
    if tunables().get_disable_sql_auto_retries() {
        return do_query().await;
    }
    Ok(retry(
        None,
        |_| do_query(),
        should_retry_mysql_query,
        // See https://fburl.com/7dmedu1u for backoff reasoning
        RetryLogic::ExponentialWithJitter {
            base: Duration::from_secs(10),
            factor: 1.2,
            jitter: Duration::from_secs(5),
        },
        RETRY_ATTEMPTS,
    )
    .await?
    .0)
}

#[cfg(test)]
mod tests {
    use super::*;

    queries_with_retry! {
        read TestQuery(param_str: String, param_uint: u64) -> (u64, Option<i32>, String, i64) {
            "SELECT 44, NULL, {param_str}, {param_uint}"
        }
        pub(crate) read TestQuery2() -> (u64, Option<String>) {
            "SELECT 44, NULL"
        }
        pub(super) write TestQuery3(values: (
            val1: i32,
        )) {
            none,
            "INSERT INTO my_table (num, str) VALUES {values}"
        }
        write TestQuery4(id: &str) {
            none,
            mysql("DELETE FROM my_table where id = {id}")
            sqlite("DELETE FROM mytable2 where id = {id}")
        }
    }

    #[allow(dead_code, unreachable_code)]
    async fn should_compile() -> Result<()> {
        TestQuery::query(todo!(), todo!(), todo!()).await?;
        TestQuery::query_with_transaction(todo!(), todo!(), todo!()).await?;
        TestQuery2::query(todo!()).await?;
        TestQuery2::query_with_transaction(todo!()).await?;
        TestQuery3::query(todo!(), &[(&12,)]).await?;
        TestQuery3::query_with_transaction(todo!(), &[(&12,)]).await?;
        TestQuery4::query(todo!(), &"hello").await?;
        Ok(())
    }
}